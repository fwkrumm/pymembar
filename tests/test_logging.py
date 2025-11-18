"""
Tests for membar logging functionality.
Tests that the logging callback system works correctly.
"""

import unittest
import sys

try:
    import membar
except ImportError as e:
    print(f"Failed to import membar module: {e}")
    print("Make sure to build and install the module first; cf. README.md.")
    sys.exit(1)


class TestMembarLogging(unittest.TestCase):
    """Test logging callback functionality."""

    def setUp(self):
        """Reset logging before each test."""
        membar.set_log_callback(None)
        self.log_messages = []

    def tearDown(self):
        """Clean up logging after each test."""
        membar.set_log_callback(None)

    def test_set_log_callback_exists(self):
        """Test that set_log_callback function exists."""
        self.assertTrue(hasattr(membar, 'set_log_callback'))
        self.assertTrue(callable(membar.set_log_callback))

    def test_set_log_callback_none(self):
        """Test that setting callback to None doesn't crash."""
        try:
            membar.set_log_callback(None)
        except Exception as e:
            self.fail(f"set_log_callback(None) raised an exception: {e}")

    def test_set_log_callback_with_function(self):
        """Test setting a logging callback function."""
        def my_logger(msg):
            self.log_messages.append(msg)

        membar.set_log_callback(my_logger)
        membar.wmb()

        # verify that logging was called
        self.assertGreater(len(self.log_messages), 0)
        self.assertIn("wmb", self.log_messages[0])

    def test_logging_all_functions(self):
        """Test that all barrier functions trigger logging."""
        def my_logger(msg):
            self.log_messages.append(msg)

        membar.set_log_callback(my_logger)

        membar.wmb()
        membar.rmb()
        membar.fence()

        # verify all three functions logged
        self.assertEqual(len(self.log_messages), 3)
        self.assertIn("wmb", self.log_messages[0])
        self.assertIn("rmb", self.log_messages[1])
        self.assertIn("fence", self.log_messages[2])

    def test_logging_disabled_by_default(self):
        """Test that logging is disabled by default."""
        # don't set any callback
        membar.wmb()
        membar.rmb()
        membar.fence()

        # no messages should be logged
        self.assertEqual(len(self.log_messages), 0)

    def test_logging_can_be_disabled(self):
        """Test that logging can be disabled after being enabled."""
        def my_logger(msg):
            self.log_messages.append(msg)

        # enable logging
        membar.set_log_callback(my_logger)
        membar.wmb()
        self.assertEqual(len(self.log_messages), 1)

        # disable logging
        membar.set_log_callback(None)
        membar.wmb()

        # message count should not increase
        self.assertEqual(len(self.log_messages), 1)

    def test_logging_callback_receives_string(self):
        """Test that callback receives string messages."""
        received_types = []

        def my_logger(msg):
            received_types.append(type(msg))
            self.log_messages.append(msg)

        membar.set_log_callback(my_logger)
        membar.wmb()

        self.assertEqual(len(received_types), 1)
        self.assertEqual(received_types[0], str)

    def test_logging_messages_contain_implementation_info(self):
        """Test that log messages contain implementation details."""
        def my_logger(msg):
            self.log_messages.append(msg)

        membar.set_log_callback(my_logger)
        membar.fence()

        # verify message contains useful information
        self.assertGreater(len(self.log_messages), 0)
        msg = self.log_messages[0].lower()

        # should mention the function name
        self.assertIn("fence", msg)

        # should mention some implementation detail
        implementation_keywords = ["c11", "msvc", "gnu", "atomic", "barrier", "bsd"]
        has_implementation_info = any(keyword in msg for keyword in implementation_keywords)
        self.assertTrue(has_implementation_info,
                       f"Log message should contain implementation info: {msg}")

    def test_invalid_callback_raises_typeerror(self):
        """Test that non-callable objects raise TypeError."""
        with self.assertRaises(TypeError):
            membar.set_log_callback("not a function")

        with self.assertRaises(TypeError):
            membar.set_log_callback(42)

        with self.assertRaises(TypeError):
            membar.set_log_callback([])

    def test_callback_can_be_replaced(self):
        """Test that callback can be replaced with a new one."""
        log1 = []
        log2 = []

        def logger1(msg):
            log1.append(msg)

        def logger2(msg):
            log2.append(msg)

        # set first logger
        membar.set_log_callback(logger1)
        membar.wmb()
        self.assertEqual(len(log1), 1)
        self.assertEqual(len(log2), 0)

        # replace with second logger
        membar.set_log_callback(logger2)
        membar.wmb()
        self.assertEqual(len(log1), 1)  # should not increase
        self.assertEqual(len(log2), 1)  # new logger should receive message

    def test_callback_with_print(self):
        """Test that print can be used as a callback."""
        # this should not crash
        try:
            membar.set_log_callback(print)
            membar.wmb()
            membar.set_log_callback(None)
        except Exception as e:
            self.fail(f"Using print as callback raised an exception: {e}")

    def test_lambda_as_callback(self):
        """Test that lambda functions work as callbacks."""
        def my_logger(msg):
            self.log_messages.append(msg)

        membar.set_log_callback(lambda msg: my_logger(msg))
        membar.fence()

        self.assertEqual(len(self.log_messages), 1)
        self.assertIn("fence", self.log_messages[0])


if __name__ == '__main__':
    unittest.main()
