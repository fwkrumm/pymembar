from setuptools import setup, Extension, find_packages
import re

def get_version():
    """Read version from membar/__init__.py"""
    with open("membar/__init__.py", "r") as f:
        content = f.read()
        match = re.search(r'^__version__\s*=\s*[\'"]([^\'"]*)[\'"]', content, re.MULTILINE)
        if match:
            return match.group(1)
    raise RuntimeError("Version string not found in membar/__init__.py")

membar_module = Extension(
    "membar._membar",  # Use underscore to indicate private/internal module
    sources=["membarmodule.c", "src/membar.c"],  # Include your actual C source files
    include_dirs=["./include"],                # Path to membar.h
)

setup(
    name="pymembar",
    version=get_version(),
    description="Python bindings for memory barriers",
    packages=find_packages(),
    package_data={"membar": ["__init__.pyi"]},
    include_package_data=True,
    ext_modules=[membar_module],
)
