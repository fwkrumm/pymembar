"""Memory barrier utilities: wmb, rmb, and fence operations."""
# pylint: disable=duplicate-code
try:
    from ._membar import wmb, rmb, fence, set_log_callback  # Import from private C extension
    __all__ = ['wmb', 'rmb', 'fence', 'set_log_callback']
except ImportError as e:
    raise ImportError(f"Could not import C extension module: {e}") from e

# do NOT alter the following line in any way EXCEPT changing
# the version number. no comments, no rename, whatsoever
__version__ = "0.1.0"
