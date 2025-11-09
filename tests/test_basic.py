"""
Basic functional tests for membar module.
Tests that the API works without crashing.
"""

import unittest
import sys
import os

try:
    import membar
except ImportError as e:
    print(f"Failed to import membar: {e}")
    print("Make sure to build the module first!")
    sys.exit(1)


class TestMembarBasic(unittest.TestCase):
    """Basic functionality tests for memory barrier functions."""

    def test_module_imports(self):
        """Test that membar module imports correctly."""
        self.assertTrue(hasattr(membar, 'wmb'))
        self.assertTrue(hasattr(membar, 'rmb'))
        self.assertTrue(hasattr(membar, 'fence'))

    def test_functions_callable(self):
        """Test that all functions are callable."""
        self.assertTrue(callable(membar.wmb))
        self.assertTrue(callable(membar.rmb))
        self.assertTrue(callable(membar.fence))

    def test_wmb_no_crash(self):
        """Test that wmb() doesn't crash."""
        try:
            membar.wmb()
        except Exception as e:
            self.fail(f"wmb() raised an exception: {e}")

    def test_rmb_no_crash(self):
        """Test that rmb() doesn't crash."""
        try:
            membar.rmb()
        except Exception as e:
            self.fail(f"rmb() raised an exception: {e}")

    def test_fence_no_crash(self):
        """Test that fence() doesn't crash."""
        try:
            membar.fence()
        except Exception as e:
            self.fail(f"fence() raised an exception: {e}")

    def test_multiple_calls(self):
        """Test multiple calls in sequence."""
        for _ in range(100):
            membar.wmb()
            membar.rmb()
            membar.fence()

    def test_return_values(self):
        """Test that functions return None."""
        self.assertIsNone(membar.wmb())
        self.assertIsNone(membar.rmb())
        self.assertIsNone(membar.fence())


if __name__ == '__main__':
    unittest.main()
