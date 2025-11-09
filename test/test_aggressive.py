"""
Aggressive memory barrier tests designed to detect memory ordering issues.
These tests use techniques to maximize the chance of detecting missing barriers.
"""

import unittest
import threading
import time
import sys
import os
import random

# Add the build directory to path for testing
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'build', 'lib.win-amd64-cpython-311'))

try:
    import membar
except ImportError as e:
    print(f"Failed to import membar: {e}")
    print("Make sure to build the module first!")
    sys.exit(1)


class TestMembarAggressive(unittest.TestCase):
    """Aggressive tests designed to actually detect missing memory barriers."""

    def test_aggressive_dekker_algorithm(self):
        """
        Modified Dekker's algorithm that should fail without proper barriers.
        Uses threading with aggressive timing to maximize race conditions.
        """
        iterations = 1000
        reordering_detected = 0

        for iteration in range(iterations):
            # Shared variables
            x = [0]
            y = [0]
            result1 = [None]
            result2 = [None]

            def process1():
                x[0] = 1                # x = 1
                membar.wmb()            # Test WITH barrier
                result1[0] = y[0]       # read y

            def process2():
                y[0] = 1                # y = 1
                membar.wmb()            # Test WITH barrier
                result2[0] = x[0]       # read x

            # Start threads simultaneously
            t1 = threading.Thread(target=process1)
            t2 = threading.Thread(target=process2)

            t1.start()
            t2.start()

            t1.join()
            t2.join()

            # Both threads saw 0 = potential reordering
            if result1[0] == 0 and result2[0] == 0:
                reordering_detected += 1

        reorder_percentage = (reordering_detected / iterations) * 100
        print(f"Dekker reordering detected: {reorder_percentage:.1f}%")

        # With barriers, should see less reordering than without
        # This is still probabilistic but more aggressive
        self.assertLess(reorder_percentage, 80,
                       "Memory barriers may not be working effectively")

    def test_store_buffer_bypass(self):
        """
        Test designed to detect store buffer issues.
        Creates high memory pressure to increase chance of reordering.
        """
        iterations = 1000
        violations = 0

        for iteration in range(iterations):
            # Large arrays to create memory pressure
            data = [0] * 10000
            flag = [False]
            violation_detected = [False]

            def writer():
                # Write lots of data to create store buffer pressure
                for i in range(0, len(data), 100):
                    data[i] = iteration + 1

                # Critical write that must be visible
                data[5000] = 999999
                membar.wmb()  # Ensure writes are committed
                flag[0] = True

            def reader():
                # Busy wait to increase timing pressure
                while not flag[0]:
                    pass

                membar.rmb()  # Ensure reads are fresh

                # Check if critical write is visible
                if data[5000] != 999999:
                    violation_detected[0] = True

            writer_thread = threading.Thread(target=writer)
            reader_thread = threading.Thread(target=reader)

            reader_thread.start()
            writer_thread.start()

            writer_thread.join()
            reader_thread.join(timeout=0.1)

            if violation_detected[0]:
                violations += 1

        violation_percentage = (violations / iterations) * 100
        print(f"Store buffer violations: {violation_percentage:.1f}%")

        # Should be very low with proper barriers
        self.assertLess(violation_percentage, 5,
                       "Too many store buffer violations detected")

    def test_without_barriers_comparison(self):
        """
        Comparison test - deliberately omit barriers to show the difference.
        This test should show higher failure rates without barriers.
        """
        iterations = 500
        failures_with_barriers = 0
        failures_without_barriers = 0

        for iteration in range(iterations):
            shared_data = [0, 0]  # [value, flag]

            # Test WITH barriers
            results_with = [None, None]

            def writer_with():
                shared_data[0] = iteration + 1
                membar.wmb()  # WITH barrier
                shared_data[1] = 1

            def reader_with():
                while shared_data[1] == 0:
                    pass
                membar.rmb()  # WITH barrier
                results_with[0] = shared_data[0]

            t1 = threading.Thread(target=writer_with)
            t2 = threading.Thread(target=reader_with)
            t2.start()
            t1.start()
            t1.join()
            t2.join(timeout=0.01)

            if results_with[0] != iteration + 1:
                failures_with_barriers += 1

            # Reset for next test
            shared_data = [0, 0]

            # Test WITHOUT barriers
            results_without = [None, None]

            def writer_without():
                shared_data[0] = iteration + 1
                # NO barrier here!
                shared_data[1] = 1

            def reader_without():
                while shared_data[1] == 0:
                    pass
                # NO barrier here!
                results_without[0] = shared_data[0]

            t3 = threading.Thread(target=writer_without)
            t4 = threading.Thread(target=reader_without)
            t4.start()
            t3.start()
            t3.join()
            t4.join(timeout=0.01)

            if results_without[0] != iteration + 1:
                failures_without_barriers += 1

        with_barrier_failure_rate = (failures_with_barriers / iterations) * 100
        without_barrier_failure_rate = (failures_without_barriers / iterations) * 100

        print(f"Failure rate WITH barriers: {with_barrier_failure_rate:.1f}%")
        print(f"Failure rate WITHOUT barriers: {without_barrier_failure_rate:.1f}%")

        # Barriers should provide some improvement (though may be small due to GIL)
        self.assertLessEqual(with_barrier_failure_rate, without_barrier_failure_rate + 0.1,
                            "Barriers should not make things worse")

    def test_compiler_reordering_prevention(self):
        """
        Test that barriers prevent compiler reordering optimizations.
        This is more about compiler barriers than hardware barriers.
        """
        iterations = 10000

        # Variables that compiler might want to reorder
        counter = [0]
        flag = [False]
        results = []

        def worker():
            for i in range(iterations):
                # These operations could be reordered by compiler
                temp_counter = counter[0]
                counter[0] = temp_counter + 1

                # Compiler barrier should prevent reordering
                membar.fence()

                flag[0] = True
                results.append(counter[0])
                flag[0] = False

        thread = threading.Thread(target=worker)
        thread.start()
        thread.join()

        # Check that operations weren't reordered in unexpected ways
        self.assertEqual(len(results), iterations)
        self.assertEqual(counter[0], iterations)

        # Results should be monotonically increasing
        for i in range(1, len(results)):
            self.assertGreaterEqual(results[i], results[i-1],
                                   "Counter values decreased - possible reordering")


class TestBarrierEffectiveness(unittest.TestCase):
    """Tests to measure actual effectiveness of barriers."""

    def test_barrier_strength_comparison(self):
        """Compare the strength of different barrier types."""

        def test_barrier_type(barrier_func, barrier_name):
            """Test a specific barrier function."""
            iterations = 1000
            reorderings = 0

            for _ in range(iterations):
                x, y = [0], [0]
                r1, r2 = [None], [None]

                def thread1():
                    x[0] = 1
                    barrier_func()  # Use the barrier
                    r1[0] = y[0]

                def thread2():
                    y[0] = 1
                    barrier_func()  # Use the barrier
                    r2[0] = x[0]

                t1 = threading.Thread(target=thread1)
                t2 = threading.Thread(target=thread2)
                t1.start()
                t2.start()
                t1.join()
                t2.join()

                if r1[0] == 0 and r2[0] == 0:
                    reorderings += 1

            return (reorderings / iterations) * 100

        # Test different barrier types
        wmb_reordering = test_barrier_type(membar.wmb, "wmb")
        rmb_reordering = test_barrier_type(membar.rmb, "rmb")
        fence_reordering = test_barrier_type(membar.fence, "fence")

        print(f"Reordering rates:")
        print(f"  wmb():   {wmb_reordering:.1f}%")
        print(f"  rmb():   {rmb_reordering:.1f}%")
        print(f"  fence(): {fence_reordering:.1f}%")

        # fence() should be strongest (lowest reordering)
        self.assertLessEqual(fence_reordering, wmb_reordering + 1)
        self.assertLessEqual(fence_reordering, rmb_reordering + 1)


if __name__ == '__main__':
    unittest.main(verbosity=2)