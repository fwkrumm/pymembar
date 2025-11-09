from setuptools import setup, Extension, find_packages

membar_module = Extension(
    "membar._membar",  # Use underscore to indicate private/internal module
    sources=["membarmodule.c", "src/membar.c"],  # Include your actual C source files
    include_dirs=["./include"],                # Path to membar.h
)

setup(
    name="pymembar",
    version="1.0",
    description="Python bindings for memory barriers",
    packages=find_packages(),
    py_modules=["membar"],
    package_data={"membar": ["__init__.pyi"]},
    include_package_data=True,
    ext_modules=[membar_module],
)
