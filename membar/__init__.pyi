"""Type stubs for pymembar - Memory barrier utilities for Python."""

__version__: str

def wmb() -> None:
    """
    Write Memory Barrier.
    
    Ensures that all write operations issued before this barrier are completed
    before any write operations issued after this barrier.
    """
    ...

def rmb() -> None:
    """
    Read Memory Barrier.
    
    Ensures that all read operations issued before this barrier are completed
    before any read operations issued after this barrier.
    """
    ...

def fence() -> None:
    """
    Full Memory Fence.
    
    Ensures that all memory operations (both reads and writes) issued before
    this barrier are completed before any memory operations issued after this barrier.
    """
    ...

__all__ = ['wmb', 'rmb', 'fence']
