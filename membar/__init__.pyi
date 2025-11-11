"""
Type stubs for membar module - Memory barrier utilities for Python
"""

def wmb() -> None:
    """
    Write memory barrier.

    Ensures that all write operations issued before this barrier
    are completed before any write operations issued after this barrier.
    """
    ...

def rmb() -> None:
    """
    Read memory barrier.

    Ensures that all read operations issued before this barrier
    are completed before any read operations issued after this barrier.
    """
    ...

def fence() -> None:
    """
    Full memory fence.

    Ensures that all memory operations (both reads and writes) issued before
    this barrier are completed before any memory operations issued after this barrier.
    """
    ...

__all__ = ['wmb', 'rmb', 'fence']
