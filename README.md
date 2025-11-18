# Python Memory Barrier (pymembar)

Memory barrier utilities for Python - provides low-level memory ordering primitives for concurrent programming. Usually you would not need this in Python due to GIL and generally strong memory ordering on x86/x86_64. However, on weakly-ordered architectures like ARM, memory barriers can be useful for ensuring correct visibility and ordering of memory operations when for example working with shared memory, named semaphores, or lock-free data structures.

I used AI (GitHub Copilot) to help generate parts of this README, documentation strings, parts of the core code, the tests, and some configuration files. Please report any inaccuracies or errors you may find.


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

### Basic Usage

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

### Logging Support

You can enable optional logging to see which memory barrier implementation is being used at runtime. This is useful for debugging and understanding which underlying mechanism (C11 atomics, MSVC, GNU atomics, BSD, or compiler barrier) is executing on your platform.

**⚠️ Performance Warning:** Enabling logging adds overhead to every memory barrier call (callback check and function call overhead). This can significantly impact performance in tight loops or performance-critical code paths. Logging should primarily be used for debugging and development, and should be disabled (`set_log_callback(None)`) in production code where performance is critical.

```python
import membar

# Enable logging with a custom callback
def my_logger(message):
    print(f"[MEMBAR] {message}")

membar.set_log_callback(my_logger)

# Now all barrier calls will log their implementation
membar.wmb()   # Logs: "wmb: using C11 atomic_thread_fence(memory_order_release)"
membar.fence() # Logs: "fence: using C11 atomic_thread_fence(memory_order_seq_cst)"

# You can also use print directly
membar.set_log_callback(print)

# Or integrate with Python's logging module
import logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)
membar.set_log_callback(logger.info)

# Disable logging by passing None
membar.set_log_callback(None)
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

Parts of the core code, the tests, this README, and documentation strings were generated with AI assistance.

## Release history

- 0.0.1 - Initial release
- 0.0.2 - Added ARM build support
- 0.0.3 - Added logging support and multiple synchronization implementations


## TODOs
- Is it possible to write tests for the memory barrier core functionality?
