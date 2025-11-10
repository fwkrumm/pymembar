// membarmodule.c

#include <Python.h>
#include "membar.h"

static PyObject* py_membar_wmb(PyObject* self, PyObject* args) {
    membar_wmb();
    Py_RETURN_NONE;
}

static PyObject* py_membar_rmb(PyObject* self, PyObject* args) {
    membar_rmb();
    Py_RETURN_NONE;
}

static PyObject* py_membar_fence(PyObject* self, PyObject* args) {
    membar_fence();
    Py_RETURN_NONE;
}

static PyMethodDef MembarMethods[] = {
    {"wmb", py_membar_wmb, METH_NOARGS, "Write memory barrier"},
    {"rmb", py_membar_rmb, METH_NOARGS, "Read memory barrier"},
    {"fence", py_membar_fence, METH_NOARGS, "Full memory fence"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef membarmodule = {
    PyModuleDef_HEAD_INIT,
    "_membar",                                 // module name should match the extension name
    "Memory barrier utilities for Python",     // module docstring
    -1,                                        // size of per-interpreter state or -1
    MembarMethods
};

PyMODINIT_FUNC PyInit__membar(void) {          // function name must match extension name
    return PyModule_Create(&membarmodule);
}
