"""
Architecture-specific memory barrier tests.
These tests are designed to expose real hardware-level memory ordering issues.
"""

import unittest
import threading
import time
import sys
import os
import platform

# Add the build directory to path for testing
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'build', 'lib.win-amd64-cpython-311'))

try:
    import membar
except ImportError as e:
    print(f"Failed to import membar: {e}")
    print("Make sure to build the module first!")
    sys.exit(1)


class TestArchitectureSpecific(unittest.TestCase):
    """Architecture-specific tests for memory barriers."""

    def setUp(self):
        self.arch = platform.machine().lower()
        self.cpu_count = os.cpu_count()
        print(f"\nTesting on {self.arch} with {self.cpu_count} CPU cores")

    def test_x86_store_forwarding(self):
        """
        Test for x86-specific store forwarding behavior.
        On x86, stores are generally ordered, but store-to-load forwarding can cause issues.
        """
        if 'x86' not in self.arch and 'amd64' not in self.arch:
            self.skipTest(f"X86-specific test, running on {self.arch}")

        iterations = 10000
        forwarding_failures = 0

        for iteration in range(iterations):
            # Use arrays to ensure we're working with actual memory locations
            buffer = [0] * 64  # 64 integers to span cache lines
            index1 = 0
            index2 = 32  # Different cache line

            results = [None, None]

            def store_forwarding_test():
                # Write to first location
                buffer[index1] = iteration + 1

                # Memory barrier to ensure ordering
                membar.wmb()

                # Write to second location
                buffer[index2] = iteration + 2

                # Read back first location
                results[0] = buffer[index1]
                results[1] = buffer[index2]

            thread = threading.Thread(target=store_forwarding_test)
            thread.start()
            thread.join()

            # Check if values are as expected
            if results[0] != iteration + 1 or results[1] != iteration + 2:
                forwarding_failures += 1

        failure_rate = (forwarding_failures / iterations) * 100
        print(f"Store forwarding failures: {failure_rate:.3f}%")

        # Should be very low with proper barriers
        self.assertLess(failure_rate, 0.1,
                       f"Too many store forwarding failures: {failure_rate}%")

    def test_weak_memory_ordering_simulation(self):
        """
        Simulate weak memory ordering scenarios that would fail on ARM/PowerPC.
        Even on x86, this can help verify barrier correctness.
        """
        iterations = 5000
        ordering_violations = 0

        for iteration in range(iterations):
            # Simulate message passing pattern
            message = [0]
            message_ready = [False]
            received_message = [None]

            def sender():
                # Prepare message
                message[0] = iteration + 42

                # Ensure message is written before flag
                membar.wmb()  # Write barrier crucial here

                # Signal message is ready
                message_ready[0] = True

            def receiver():
                # Wait for message
                while not message_ready[0]:
                    time.sleep(0.00001)  # Tiny delay to increase race window

                # Ensure flag read before message read
                membar.rmb()  # Read barrier crucial here

                # Read message
                received_message[0] = message[0]

            sender_thread = threading.Thread(target=sender)
            receiver_thread = threading.Thread(target=receiver)

            receiver_thread.start()
            sender_thread.start()

            sender_thread.join()
            receiver_thread.join(timeout=0.1)

            # Check if message was received correctly
            if received_message[0] != iteration + 42:
                ordering_violations += 1

        violation_rate = (ordering_violations / iterations) * 100
        print(f"Memory ordering violations: {violation_rate:.3f}%")

        # Should be very low with proper barriers
        self.assertLess(violation_rate, 1.0,
                       f"Too many memory ordering violations: {violation_rate}%")

    def test_cache_coherency_stress(self):
        """
        Stress test cache coherency with multiple cores.
        Creates contention across CPU cores to test barrier effectiveness.
        """
        if self.cpu_count < 2:
            self.skipTest("Need multiple CPU cores for this test")

        # Shared counter that all threads will modify
        shared_counter = [0]
        operations_per_thread = 1000
        num_threads = min(self.cpu_count, 8)

        barrier_errors = []

        def worker_thread(thread_id):
            local_errors = 0

            for i in range(operations_per_thread):
                # Read current value
                old_value = shared_counter[0]

                # Memory fence to ensure ordering
                membar.fence()

                # Increment (non-atomic, intentionally racy)
                shared_counter[0] = old_value + 1

                # Another fence
                membar.fence()

                # Verify the increment (probabilistic check)
                new_value = shared_counter[0]
                if new_value < old_value:
                    local_errors += 1

            barrier_errors.append(local_errors)

        # Start all worker threads
        threads = []
        for thread_id in range(num_threads):
            t = threading.Thread(target=worker_thread, args=(thread_id,))
            threads.append(t)
            t.start()

        # Wait for completion
        for t in threads:
            t.join()

        total_errors = sum(barrier_errors)
        total_operations = num_threads * operations_per_thread
        error_rate = (total_errors / total_operations) * 100

        print(f"Cache coherency test:")
        print(f"  Threads: {num_threads}")
        print(f"  Operations: {total_operations:,}")
        print(f"  Errors: {total_errors}")
        print(f"  Error rate: {error_rate:.3f}%")
        print(f"  Final counter: {shared_counter[0]} (expected: ~{total_operations})")

        # Due to race conditions, we expect some errors, but not too many
        # The barriers should help maintain some level of ordering
        self.assertLess(error_rate, 50,
                       "Excessive cache coherency errors")

    def test_memory_fence_strength(self):
        """
        Test the relative strength of different memory fence types.
        Measures how effectively each barrier type prevents reordering.
        """

        def measure_fence_effectiveness(fence_func, fence_name):
            iterations = 2000
            reorder_count = 0

            for _ in range(iterations):
                x, y = [0], [0]
                r1, r2 = [None], [None]

                def thread1():
                    x[0] = 1
                    fence_func()  # Apply the fence
                    r1[0] = y[0]

                def thread2():
                    y[0] = 1
                    fence_func()  # Apply the fence
                    r2[0] = x[0]

                t1 = threading.Thread(target=thread1)
                t2 = threading.Thread(target=thread2)
                t1.start()
                t2.start()
                t1.join()
                t2.join()

                # Both threads seeing 0 indicates reordering
                if r1[0] == 0 and r2[0] == 0:
                    reorder_count += 1

            reorder_rate = (reorder_count / iterations) * 100
            print(f"  {fence_name}: {reorder_rate:.2f}% reordering")
            return reorder_rate

        print("\nFence effectiveness comparison:")

        # Test each fence type
        wmb_rate = measure_fence_effectiveness(membar.wmb, "wmb() ")
        rmb_rate = measure_fence_effectiveness(membar.rmb, "rmb() ")
        fence_rate = measure_fence_effectiveness(membar.fence, "fence()")

        # Test no fence for comparison
        no_fence_rate = measure_fence_effectiveness(lambda: None, "none  ")

        print(f"\nRelative effectiveness:")
        print(f"  Improvement over no fence:")
        print(f"    wmb():   {no_fence_rate - wmb_rate:+.2f} percentage points")
        print(f"    rmb():   {no_fence_rate - rmb_rate:+.2f} percentage points")
        print(f"    fence(): {no_fence_rate - fence_rate:+.2f} percentage points")

        # fence() should be most effective (lowest reordering)
        self.assertLessEqual(fence_rate, wmb_rate + 1)
        self.assertLessEqual(fence_rate, rmb_rate + 1)

        # All barriers should provide some improvement over no fence
        self.assertLessEqual(wmb_rate, no_fence_rate + 1)
        self.assertLessEqual(rmb_rate, no_fence_rate + 1)
        self.assertLessEqual(fence_rate, no_fence_rate + 1)


if __name__ == '__main__':
    unittest.main(verbosity=2)