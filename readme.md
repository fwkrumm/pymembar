# Python Memory Barrier (pymembar)

Memory barrier utilities for Python - provides low-level memory ordering primitives for concurrent programming. Usually you would not need this in Python due to GIL and generally strong memory ordering on x86/x86_64. However, on weakly-ordered architectures like ARM, memory barriers can be useful for ensuring correct visibility and ordering of memory operations when for example working with shared memory, named semaphores, or lock-free data structures.

I used AI (GitHub Copilot) to help generate parts of this README, documentation strings, and some configuration files. Please report any inaccuracies or errors you may find.


## Overview

This package provides Python bindings for memory barrier operations, ensuring that memory operations are visible to other processes and threads in the correct order. Memory barriers do not provide synchronization themselves, but rather control the visibility and ordering of memory operations across CPU cores and processes.

The package includes three core functions:

- `wmb()` - Write memory barrier
- `rmb()` - Read memory barrier
- `fence()` - Full memory fence

## Installation

```bash
pip install pymembar
```

## Usage

```python
import membar

# Write memory barrier - ensures all write operations before this point
# are visible to other processes/threads before any writes after this point
membar.wmb()

# Read memory barrier - ensures all read operations before this point
# complete before any read operations after this point
membar.rmb()

# Full memory fence - ensures all memory operations before this point
# are visible to other processes/threads before any operations after this point
membar.fence()
```

## When Are Memory Barriers Needed?

Memory barriers are primarily needed on weakly-ordered CPU architectures like ARM, where the processor may reorder memory operations for performance optimization. On x86/x86_64 architectures, the strong memory ordering model means that memory barriers are often not strictly necessary for most use cases, as the hardware already provides strong ordering guarantees.

However, memory barriers can still be useful on x86 in specific scenarios:
- When interfacing with memory-mapped I/O
- In lock-free programming with specific compiler optimizations
- When precise ordering is critical for correctness

**Note:** Most Python applications will not need memory barriers due to the Global Interpreter Lock (GIL) and Python's threading model.

## Requirements

- Python 3.8+
- Compatible with Windows, Linux, and macOS

## Contributing

I am not a C expert and would be happy to receive any constructive feedback, suggestions, or contributions to improve this library. Feel free to open issues or submit pull requests on the [GitHub repository](https://github.com/fwkrumm/pymembar).

## Disclaimer

Parts of the core code, this README, and documentation strings were generated with AI assistance.


# TODOs
- Is it possible to write tests for the functionality
- Add ARM build to CI/CD pipeline as soon as they are available via github actions
