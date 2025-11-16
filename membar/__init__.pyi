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

def set_log_callback(callback: Optional[Callable[[str], None]]) -> None:
    """
    set optional logging callback.

    Args:
        callback: A callable that takes a string message, or None to disable logging.
                  The callback will be invoked whenever a memory barrier function is called,
                  with information about which implementation is being used.

    Example:
        >>> import membar
        >>> membar.set_log_callback(print)
        >>> membar.wmb()                        # will log which implementation is used
        >>> membar.set_log_callback(None)       # disable logging

    Or use a custom logger:
        >>> import logging
        >>> logger = logging.getLogger("membar_logger")
        >>> membar.set_log_callback(logger.info)        # logging level set callback type
        >>> membar.rmb()                                # logs via the logger
        >>> membar.set_log_callback(None)               # disable logging
    """
    ...

__all__ = ['wmb', 'rmb', 'fence', 'set_log_callback']
