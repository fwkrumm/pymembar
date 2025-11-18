// membarmodule.c

#include <Python.h>
#include "membar.h"

#ifdef _WIN32
#include <windows.h>
static CRITICAL_SECTION callback_lock;
static int lock_initialized = 0;
#else
#include <pthread.h>
static pthread_mutex_t callback_lock;
static int lock_initialized = 0;
#endif

/**
 * Python wrapper for wmb (write memory barrier)
 * Calls the C implementation and returns None to Python
 *
 * @param self - module instance (unused)
 * @param args - Python arguments (unused, function takes no arguments)
 * @return Py_None
 */
static PyObject* py_membar_wmb(PyObject* self, PyObject* args) {
    membar_wmb();
    Py_RETURN_NONE;
}

/**
 * Python wrapper for rmb (read memory barrier)
 * Calls the C implementation and returns None to Python
 *
 * @param self - module instance (unused)
 * @param args - Python arguments (unused, function takes no arguments)
 * @return Py_None
 */
static PyObject* py_membar_rmb(PyObject* self, PyObject* args) {
    membar_rmb();
    Py_RETURN_NONE;
}

/**
 * Python wrapper for fence (full memory fence)
 * Calls the C implementation and returns None to Python
 *
 * @param self - module instance (unused)
 * @param args - Python arguments (unused, function takes no arguments)
 * @return Py_None
 */
static PyObject* py_membar_fence(PyObject* self, PyObject* args) {
    membar_fence();
    Py_RETURN_NONE;
}

/* Python callback wrapper for logging */
static PyObject* python_log_callback = NULL;

/**
 * C callback wrapper that invokes the Python logging callback
 * This function is called from C code and bridges to Python
 * Acquires the GIL before calling Python code for thread safety
 *
 * @param message - the log message string to pass to Python callback
 */
static void log_callback_wrapper(const char* message) {
    PyObject* callback_snapshot;

    // Acquire mutex to safely read the callback pointer
#ifdef _WIN32
    EnterCriticalSection(&callback_lock);
#else
    pthread_mutex_lock(&callback_lock);
#endif

    callback_snapshot = python_log_callback;
    if (callback_snapshot != NULL) {
        Py_INCREF(callback_snapshot);  // hold a reference while we use it
    }

#ifdef _WIN32
    LeaveCriticalSection(&callback_lock);
#else
    pthread_mutex_unlock(&callback_lock);
#endif

    // Now invoke the callback outside the mutex (but inside the GIL)
    if (callback_snapshot != NULL) {
        PyGILState_STATE gstate = PyGILState_Ensure();

        PyObject* result = PyObject_CallFunction(callback_snapshot, "s", message);
        if (result == NULL) {
            PyErr_Clear();  // do not let logging errors propagate
        } else {
            Py_DECREF(result);
        }

        Py_DECREF(callback_snapshot);  // release our temporary reference
        PyGILState_Release(gstate);
    }
}

/**
 * Python wrapper for set_log_callback
 * Sets or clears the logging callback function
 * Protected by mutex to prevent race conditions in reference counting
 *
 * GIL SAFETY: This function is called from Python with the GIL already held (guaranteed
 * for all Python C API functions). All Py_INCREF/Py_XDECREF operations are safe because
 * they execute while the GIL is held. The mutex only protects against concurrent calls
 * to this setter function, not GIL-protected operations.
 *
 * @param self - module instance (unused)
 * @param args - Python arguments tuple containing the callback function or None
 * @return Py_None on success, NULL on error
 */
static PyObject* py_membar_set_log_callback(PyObject* self, PyObject* args) {
    PyObject* callback;
    PyObject* old_callback;

    // GIL is held throughout this function (Python C API guarantee)

    // PyArg_ParseTuple: extract Python arguments into C variables
    // "O" format = accept any Python object
    // Returns 0 on failure, setting a Python exception
    if (!PyArg_ParseTuple(args, "O", &callback)) {
        return NULL;  // propagate the exception to Python
    }

    // handle None to disable logging
    if (callback == Py_None) {
        callback = NULL;  // treat None as NULL for cleaner code below
    } else if (!PyCallable_Check(callback)) {
        // PyCallable_Check: verify the object can be called like a function
        // Returns 1 if callable, 0 if not
        // PyErr_Format: set a Python exception with formatted message including actual type
        PyErr_Format(PyExc_TypeError,
                     "callback must be callable or None, got %s",
                     Py_TYPE(callback)->tp_name);
        return NULL;  // returning NULL signals an exception occurred
    }

    // Acquire mutex to protect concurrent access to python_log_callback
    // Note: GIL is still held, so all Python C API calls remain safe
#ifdef _WIN32
    EnterCriticalSection(&callback_lock);
#else
    pthread_mutex_lock(&callback_lock);
#endif

    // atomically swap callbacks and manage reference counting
    // Py_INCREF is safe here because GIL is held (required for all refcount operations)
    old_callback = python_log_callback;
    python_log_callback = callback;

    if (callback != NULL) {
        Py_INCREF(callback);  // increase ref count so Python won't GC it (GIL held)
    }

#ifdef _WIN32
    LeaveCriticalSection(&callback_lock);
#else
    pthread_mutex_unlock(&callback_lock);
#endif

    // Py_XDECREF: safely decrease reference count of old callback (GIL still held)
    // (X variant is safe even if old_callback is NULL)
    Py_XDECREF(old_callback);

    // register/unregister our C wrapper function at the C level
    membar_set_log_callback(callback != NULL ? log_callback_wrapper : NULL);

    Py_RETURN_NONE;  // return None to Python (success)
}

static PyMethodDef MembarMethods[] = {
    {"wmb", py_membar_wmb, METH_NOARGS, "Write memory barrier"},
    {"rmb", py_membar_rmb, METH_NOARGS, "Read memory barrier"},
    {"fence", py_membar_fence, METH_NOARGS, "Full memory fence"},
    {"set_log_callback", py_membar_set_log_callback, METH_VARARGS, "Set logging callback function or None to disable"},
    {NULL, NULL, 0, NULL}
};

/**
 * Module cleanup function
 * Called when the module is being unloaded
 *
 * CRITICAL: Unregister the C callback FIRST to ensure no new calls to log_callback_wrapper
 * can start after this point. Any in-flight callbacks will complete safely because they
 * hold a reference to the callback object and we don't destroy the mutex until after
 * unregistering.
 */
static void membar_module_free(void* m) {
    // Check if initialization succeeded - if not, nothing to clean up
    // This prevents undefined behavior if PyInit__membar failed
    if (!lock_initialized) {
        return;
    }

    // STEP 1: Unregister C callback to stop new barrier functions from invoking wrapper
    // This is atomic at the C level (see src/membar.c) so it's safe to call outside mutex
    membar_set_log_callback(NULL);

    // STEP 2: Acquire GIL and mutex, then clean up Python callback reference
    // CRITICAL: Module cleanup functions (m_free) are not guaranteed to be called with
    // the GIL held, but Py_XDECREF requires the GIL. We must acquire it explicitly.
    // Any in-flight log_callback_wrapper calls will complete because they already
    // have their snapshot and reference count.
    PyGILState_STATE gstate = PyGILState_Ensure();

#ifdef _WIN32
    EnterCriticalSection(&callback_lock);
#else
    pthread_mutex_lock(&callback_lock);
#endif

    Py_XDECREF(python_log_callback);  // Safe: GIL is held
    python_log_callback = NULL;

#ifdef _WIN32
    LeaveCriticalSection(&callback_lock);
#else
    pthread_mutex_unlock(&callback_lock);
#endif

    PyGILState_Release(gstate);

    // STEP 3: Now safe to destroy mutex - no more wrapper calls can occur
#ifdef _WIN32
    DeleteCriticalSection(&callback_lock);
#else
    pthread_mutex_destroy(&callback_lock);
#endif
    lock_initialized = 0;
}

static struct PyModuleDef membarmodule = {
    PyModuleDef_HEAD_INIT,
    "_membar",                                 // module name should match the extension name
    "Memory barrier utilities for Python",     // module docstring
    -1,                                        // size of per-interpreter state or -1
    MembarMethods,
    NULL,                                      // m_slots
    NULL,                                      // m_traverse
    NULL,                                      // m_clear
    membar_module_free                         // m_free - cleanup function
};

/**
 * Module initialization function
 * Called when the _membar module is imported in Python
 * Initializes the mutex for thread-safe callback management
 *
 * THREAD SAFETY NOTE: The lock_initialized check is not itself atomic, but this is safe
 * because Python's import system holds the import lock (GIL) during module initialization.
 * PyInit__membar() is never called concurrently from multiple threads - Python guarantees
 * serialized access during the import process. This means the check-and-initialize pattern
 * is safe despite not using atomic operations.
 *
 * @return PyObject* - the initialized module object
 */
PyMODINIT_FUNC PyInit__membar(void) {          // function name must match extension name
    PyObject* module;

    // Initialize the mutex on first import (or after module reload)
    // Safe due to Python's import lock protecting this function
    if (!lock_initialized) {
#ifdef _WIN32
        InitializeCriticalSection(&callback_lock);
#else
        pthread_mutex_init(&callback_lock, NULL);
#endif
        lock_initialized = 1;
    }

    module = PyModule_Create(&membarmodule);

    // If module creation failed, clean up the mutex to prevent resource leak
    if (module == NULL) {
#ifdef _WIN32
        DeleteCriticalSection(&callback_lock);
#else
        pthread_mutex_destroy(&callback_lock);
#endif
        lock_initialized = 0;
    }

    return module;
}
