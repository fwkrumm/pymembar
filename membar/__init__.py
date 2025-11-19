# Import the C extension module and expose its functions
try:
    from ._membar import wmb, rmb, fence, set_log_callback  # Import from private C extension
    __all__ = ['wmb', 'rmb', 'fence', 'set_log_callback']
except ImportError as e:
    # Fallback error message if the C extension cannot be imported
    raise ImportError(f"Could not import C extension module: {e}")

# do NOT alter the following line in any way EXCEPT changing
# the version number. no comments, no rename, whatsoever
__version__ = "0.0.4"
