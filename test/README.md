# Memory Barrier Test Suite

> **Note:** This test suite is AI-generated. While designed to detect real memory barrier behavior and ordering issues, the tests may not cover all edge cases or platform-specific behaviors. Constructive feedback on test quality, effectiveness, and memory barrier detection is welcome via GitHub issues or pull requests.

This directory contains comprehensive tests for the `membar` Python module, which provides low-level memory barrier operations.

## Test Organization

### test/test_basic.py
Basic functionality tests that verify the module imports correctly and functions are callable:
- Module import verification
- Function availability and callability
- Return value validation
- Basic crash testing

### test/test_simple.py
Performance and reliability tests with simple scenarios:
- Performance overhead measurement
- Threading safety validation
- Message passing patterns
- Timing consistency checks

### test/test_aggressive.py
Advanced tests designed to detect memory ordering issues:
- Dekker's algorithm variant
- Store buffer bypass detection
- Compiler reordering prevention
- Barrier strength comparison

### test/test_hardware.py
Architecture-specific tests for real hardware behavior:
- X86 store forwarding tests
- Weak memory ordering simulation
- Cache coherency stress testing
- Memory fence effectiveness comparison

## Running Tests

### Run All Tests
```bash
python -m unittest discover test -v
```

### Run Specific Test Modules
```bash
# Basic tests only
python -m unittest test.test_basic -v

# Performance tests
python -m unittest test.test_simple -v

# Advanced memory ordering tests
python -m unittest test.test_aggressive -v

# Hardware-specific tests
python -m unittest test.test_hardware -v
```

### Run Individual Test Classes
```bash
# Just the basic functionality tests
python -m unittest test.test_basic.TestMembarBasic -v

# Just performance measurements
python -m unittest test.test_simple.TestMembarPerformance -v
```

## Prerequisites

1. **Build the module first:**
   ```bash
   python setup.py build_ext --inplace
   ```

2. **Install dependencies:**
   ```bash
   pip install -r requirements.txt  # if you have one
   ```

## Test Environment

These tests are designed to work on Windows x64 with Python 3.11, but should adapt to other platforms. The tests automatically detect:

- CPU architecture (x86, ARM, etc.)
- Number of CPU cores
- Python version
- Platform-specific behavior

## Performance Notes

- **test_basic.py**: Runs in milliseconds
- **test_simple.py**: Runs in 1-3 seconds (includes timing measurements)
- **test_aggressive.py**: Runs in 5-15 seconds (stress testing)
- **test_hardware.py**: Runs in 10-30 seconds (extensive hardware testing)

## Understanding Results

### Memory Barrier Overhead
Expect 5-10x performance overhead when barriers are active. This proves the barriers are executing real CPU instructions.

### Reordering Detection
On strongly-ordered architectures like x86, reordering rates should be very low (< 1%). On weakly-ordered architectures, properly working barriers should still keep rates low.

**Important:** Memory ordering failures are very unlikely on x86 architecture due to its strong memory model. These tests are primarily designed to detect issues on weakly-ordered architectures like ARM, PowerPC, or RISC-V where memory reordering is more common. Running these tests on x86 serves mainly as a baseline and to verify the barriers don't cause crashes or excessive overhead.

### Threading Tests
Message passing tests should achieve 100% success rate with proper barriers.

## Troubleshooting

**Import Errors:**
- Ensure the module is built: `python setup.py build_ext --inplace`
- Check Python path includes the build directory

**Test Failures:**
- Hardware-specific tests may fail on some architectures
- Timing-sensitive tests might fail on heavily loaded systems
- Multiprocessing tests are disabled on Windows due to GIL limitations

**Performance Issues:**
- Aggressive tests can be CPU-intensive
- Use smaller iteration counts if tests take too long
- Some tests use all available CPU cores

## Memory Barrier Reference

- **wmb()**: Write memory barrier - ensures write ordering
- **rmb()**: Read memory barrier - ensures read ordering
- **fence()**: Full memory fence - ensures all memory ordering