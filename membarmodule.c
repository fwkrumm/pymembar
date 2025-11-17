// membarmodule.c

#include <Python.h>
#include "membar.h"

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

    // Acquire the GIL before accessing python_log_callback for thread safety
    PyGILState_STATE gstate = PyGILState_Ensure();

    if (python_log_callback != NULL) {

        PyObject* result = PyObject_CallFunction(python_log_callback, "s", message);
        if (result == NULL) {
            PyErr_Clear();  // do not let logging errors propagate
        } else {
            Py_DECREF(result);
        }

        // Release the GIL after Python code completes
        PyGILState_Release(gstate);
    }
}

/**
 * Python wrapper for set_log_callback
 * Sets or clears the logging callback function
 *
 * @param self - module instance (unused)
 * @param args - Python arguments tuple containing the callback function or None
 * @return Py_None on success, NULL on error
 */
static PyObject* py_membar_set_log_callback(PyObject* self, PyObject* args) {
    PyObject* callback;

    // PyArg_ParseTuple: extract Python arguments into C variables
    // "O" format = accept any Python object
    // Returns 0 on failure, setting a Python exception
    if (!PyArg_ParseTuple(args, "O", &callback)) {
        return NULL;  // propagate the exception to Python
    }

    // handle None to disable logging
    if (callback == Py_None) {
        // Py_XDECREF: safely decrease reference count of old callback
        // (X variant is safe even if python_log_callback is NULL)
        Py_XDECREF(python_log_callback);
        python_log_callback = NULL;

        // disable logging at C level by passing NULL
        membar_set_log_callback(NULL);
        Py_RETURN_NONE;
    }

    // PyCallable_Check: verify the object can be called like a function
    // Returns 1 if callable, 0 if not
    if (!PyCallable_Check(callback)) {
        // PyErr_SetString: set a Python exception that will be raised
        PyErr_SetString(PyExc_TypeError, "callback must be callable or None");
        return NULL;  // returning NULL signals an exception occurred
    }

    // store new callback and manage reference counting
    Py_XDECREF(python_log_callback);   // release old callback (if any)
    Py_INCREF(callback);               // increase ref count so Python won't GC it
    python_log_callback = callback;    // store the callback for later use

    // register our C wrapper function that will call the Python callback
    membar_set_log_callback(log_callback_wrapper);

    Py_RETURN_NONE;  // return None to Python (success)
}

static PyMethodDef MembarMethods[] = {
    {"wmb", py_membar_wmb, METH_NOARGS, "Write memory barrier"},
    {"rmb", py_membar_rmb, METH_NOARGS, "Read memory barrier"},
    {"fence", py_membar_fence, METH_NOARGS, "Full memory fence"},
    {"set_log_callback", py_membar_set_log_callback, METH_VARARGS, "Set logging callback function or None to disable"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef membarmodule = {
    PyModuleDef_HEAD_INIT,
    "_membar",                                 // module name should match the extension name
    "Memory barrier utilities for Python",     // module docstring
    -1,                                        // size of per-interpreter state or -1
    MembarMethods
};

/**
 * Module initialization function
 * Called when the _membar module is imported in Python
 *
 * @return PyObject* - the initialized module object
 */
PyMODINIT_FUNC PyInit__membar(void) {          // function name must match extension name
    return PyModule_Create(&membarmodule);
}
