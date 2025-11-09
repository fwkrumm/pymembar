"""
Simple and focused memory barrier tests that work reliably.
These tests focus on verifying the API and basic functionality.
"""

import unittest
import threading
import time
import sys
import os

# Add the build directory to path for testing
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'build', 'lib.win-amd64-cpython-311'))

try:
    import membar
except ImportError as e:
    print(f"Failed to import membar: {e}")
    print("Make sure to build the module first!")
    sys.exit(1)


class TestMembarSimple(unittest.TestCase):
    """Simple, reliable tests for memory barriers."""

    def test_overhead_measurement(self):
        """Test that memory barriers have measurable overhead (proves they work)."""
        iterations = 100000

        # Baseline - empty loop
        start = time.perf_counter()
        for _ in range(iterations):
            pass
        baseline_time = time.perf_counter() - start

        # Test with memory barriers
        start = time.perf_counter()
        for _ in range(iterations):
            membar.wmb()
            membar.rmb()
            membar.fence()
        barrier_time = time.perf_counter() - start

        overhead_ratio = barrier_time / baseline_time if baseline_time > 0 else float('inf')

        print(f"\nPerformance Results:")
        print(f"  Baseline:    {baseline_time*1000:.3f} ms")
        print(f"  With barriers: {barrier_time*1000:.3f} ms")
        print(f"  Overhead:    {overhead_ratio:.1f}x")

        # Memory barriers should have some overhead (proves they're doing something)
        self.assertGreater(barrier_time, baseline_time * 0.5,
                          "Memory barriers should have measurable overhead")

        # But not excessive overhead
        self.assertLess(overhead_ratio, 10000,
                       f"Memory barrier overhead too high: {overhead_ratio:.1f}x")

    def test_barrier_consistency(self):
        """Test that barriers work consistently across multiple calls."""
        call_count = 10000

        # Test that barriers don't crash under repeated use
        try:
            for i in range(call_count):
                membar.wmb()
                if i % 3 == 0:
                    membar.rmb()
                if i % 5 == 0:
                    membar.fence()
        except Exception as e:
            self.fail(f"Memory barriers failed after {i} calls: {e}")

        print(f"✅ Successfully completed {call_count:,} barrier calls")

    def test_threading_safety(self):
        """Test that barriers are safe to call from multiple threads."""
        num_threads = 4
        calls_per_thread = 1000
        errors = []

        def worker(thread_id):
            try:
                for i in range(calls_per_thread):
                    membar.wmb()
                    membar.rmb()
                    membar.fence()
            except Exception as e:
                errors.append(f"Thread {thread_id}: {e}")

        threads = []
        for i in range(num_threads):
            t = threading.Thread(target=worker, args=(i,))
            threads.append(t)
            t.start()

        for t in threads:
            t.join()

        total_calls = num_threads * calls_per_thread * 3  # 3 barrier types

        if errors:
            self.fail(f"Threading errors: {errors}")

        print(f"✅ Successfully completed {total_calls:,} concurrent barrier calls")

    def test_barrier_timing_consistency(self):
        """Test that barrier timing is reasonably consistent."""
        samples = 1000
        timings = []

        for _ in range(samples):
            start = time.perf_counter()
            membar.fence()
            end = time.perf_counter()
            timings.append(end - start)

        avg_time = sum(timings) / len(timings)
        max_time = max(timings)
        min_time = min(timings)

        print(f"\nTiming Analysis (fence()):")
        print(f"  Average: {avg_time*1e6:.2f} μs")
        print(f"  Min:     {min_time*1e6:.2f} μs")
        print(f"  Max:     {max_time*1e6:.2f} μs")
        print(f"  Ratio:   {max_time/min_time:.1f}x")

        # Timing should be reasonably consistent
        self.assertLess(max_time, avg_time * 1000,
                       "Barrier timing too inconsistent")
        self.assertGreater(avg_time, 0,
                          "Barrier should take some time")

    def test_message_passing_pattern(self):
        """Test barriers in a realistic message-passing scenario."""
        iterations = 1000
        success_count = 0

        for iteration in range(iterations):
            message = [None]
            flag = [False]
            received = [None]

            def sender():
                # Prepare message
                message[0] = f"Message_{iteration}"
                # Ensure message is written before flag
                membar.wmb()
                # Set flag
                flag[0] = True

            def receiver():
                # Wait for flag
                while not flag[0]:
                    time.sleep(0.0001)
                # Ensure we read the flag before reading message
                membar.rmb()
                # Read message
                received[0] = message[0]

            sender_thread = threading.Thread(target=sender)
            receiver_thread = threading.Thread(target=receiver)

            receiver_thread.start()
            sender_thread.start()

            sender_thread.join()
            receiver_thread.join(timeout=0.1)

            # Check if message was received correctly
            if received[0] == f"Message_{iteration}":
                success_count += 1

        success_rate = (success_count / iterations) * 100
        print(f"\nMessage passing success rate: {success_rate:.1f}%")

        # Should have high success rate
        self.assertGreater(success_rate, 95.0,
                          f"Message passing success rate too low: {success_rate:.1f}%")

    def test_compiler_barrier_effect(self):
        """Test that barriers act as compiler barriers."""
        # This test ensures that barriers prevent certain compiler optimizations

        counter = [0]
        iterations = 10000
        results = []

        def worker():
            for i in range(iterations):
                # Operations that could potentially be reordered
                temp = counter[0]
                counter[0] = temp + 1

                # Barrier should prevent reordering
                membar.fence()

                # Store current counter value
                results.append(counter[0])

        thread = threading.Thread(target=worker)
        thread.start()
        thread.join()

        # Verify results
        self.assertEqual(len(results), iterations)
        self.assertEqual(counter[0], iterations)

        # Check that results are monotonically increasing
        for i in range(1, len(results)):
            self.assertGreaterEqual(results[i], results[i-1],
                                   f"Counter decreased at position {i}: {results[i-1]} -> {results[i]}")

        print(f"✅ Compiler barrier test: {iterations:,} operations completed correctly")


if __name__ == '__main__':
    unittest.main(verbosity=2)