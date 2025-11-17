// membar.c
#include "membar.h"
#include <stddef.h>

/*
 * This file implements memory barrier functions with optional logging support.
 * Thread-safe atomic operations are used for the logging callback to prevent
 * race conditions in multi-threaded environments.
 */

/*
 * Detect and enable C11 atomics if available
 * C11 provides standardized atomic operations via <stdatomic.h>
 */
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L) && !defined(__STDC_NO_ATOMICS__)
  #include <stdatomic.h>
  #define HAS_C11_ATOMICS 1
#endif

/*
 * MSVC (Microsoft Visual C++) support
 * Provides Windows-specific memory barriers and atomic intrinsics
 */
#if defined(_MSC_VER)
  #include <windows.h>   // For MemoryBarrier()
  #include <intrin.h>    // For _Interlocked* functions
  #define HAS_MSVC 1
  /* If C11 atomics aren't available, use MSVC intrinsics for thread safety */
  #if !defined(HAS_C11_ATOMICS)
    #define HAS_MSVC_ATOMICS 1
  #endif
#endif

/*
 * GCC/Clang compiler support
 * These compilers provide __atomic_* and __sync_* builtins
 */
#if !defined(HAS_C11_ATOMICS) && (defined(__GNUC__) || defined(__clang__))
  #define HAS_GNU_ATOMICS 1
  /* Enable atomic builtins for thread-safe callback operations */
  #define HAS_GNU_ATOMIC_BUILTINS 1
#endif

/*
 * Optional logging callback - protected with atomic operations for thread safety
 *
 * This variable stores a function pointer that gets called whenever a memory
 * barrier function executes. Different atomic implementations are used based
 * on platform capabilities to ensure thread-safe access.
 */
#if defined(HAS_C11_ATOMICS)
  /* C11: Use _Atomic type qualifier for atomic pointer operations */
  static _Atomic(membar_log_callback) log_callback = NULL;
#elif defined(HAS_MSVC_ATOMICS) || defined(HAS_GNU_ATOMIC_BUILTINS)
  /* MSVC/GCC: Use volatile to prevent compiler optimizations,
   * actual atomicity provided by intrinsics/builtins */
  static membar_log_callback volatile log_callback = NULL;
#else
  /* No atomic support available - this will trigger a compile error */
  static membar_log_callback log_callback = NULL;
  #error "Thread-safe callback storage not available - race conditions possible " \
    "with set_log_callback if used in multithreaded context. Disable this error in " \
    "case you want to compile anyway."
#endif

/**
 * Set or clear the logging callback function
 *
 * This function atomically updates the logging callback pointer to ensure
 * thread safety. Other threads reading the callback will see either the
 * old or new value, never a partially written pointer.
 *
 * @param callback - Function pointer to call for logging, or NULL to disable
 */
void membar_set_log_callback(membar_log_callback callback) {
#if defined(HAS_C11_ATOMICS)
    /* C11: Use atomic_store_explicit with release semantics
     * Release ensures all prior writes are visible before the store */
    atomic_store_explicit(&log_callback, callback, memory_order_release);
#elif defined(HAS_MSVC_ATOMICS)
    /* MSVC: Use _InterlockedExchangePointer for atomic pointer swap
     * This intrinsic provides full memory barrier semantics */
    _InterlockedExchangePointer((void* volatile*)&log_callback, (void*)callback);
#elif defined(HAS_GNU_ATOMIC_BUILTINS)
    /* GCC/Clang: Use __atomic_store_n with release semantics
     * __ATOMIC_RELEASE ensures proper memory ordering */
    __atomic_store_n(&log_callback, callback, __ATOMIC_RELEASE);
#else
    /* Fallback: Simple assignment (not thread-safe, should never reach here) */
    log_callback = callback;
#endif
}

/* macos and freebsd fallback */
#if defined(__APPLE__) || defined(__FreeBSD__)
  #define HAS_BSD_FALLBACK 1
#endif

/**
 * write memory barrier
 * ensures that all memory writes issued before this call are visible before any subsequent writes
 * used to enforce ordering in concurrent systems where write reordering may occur
 *
 * NOTE: If logging is enabled, there is a performance overhead from the callback check
 * and string formatting. Logging should primarily be used for debugging, not in
 * performance-critical production code.
 */
void membar_wmb(void) {
#if defined(HAS_C11_ATOMICS)
    /* C11: Atomically load the callback pointer with acquire semantics
     * Acquire ensures we see the callback and any data it depends on */
    membar_log_callback cb = atomic_load_explicit(&log_callback, memory_order_acquire);
    /* Execute the actual write memory barrier BEFORE logging
     * This ensures the barrier takes effect before any callback memory operations */
    atomic_thread_fence(memory_order_release);
    if (cb) cb("wmb: using C11 atomic_thread_fence(memory_order_release)");
#elif defined(HAS_MSVC)
    #if defined(HAS_MSVC_ATOMICS)
        /* MSVC: Use compare-exchange with (NULL, NULL) to atomically read the pointer
         * This is a trick: comparing with NULL and exchanging with NULL reads atomically */
        membar_log_callback cb = (membar_log_callback)_InterlockedCompareExchangePointer(
            (void* volatile*)&log_callback, NULL, NULL);
        /* Execute the actual write memory barrier using Windows API BEFORE logging */
        MemoryBarrier();
        if (cb) cb("wmb: using MSVC MemoryBarrier()");
    #else
        /* Execute the actual write memory barrier using Windows API BEFORE logging */
        MemoryBarrier();
        /* Fallback for MSVC without atomic support */
        if (log_callback) log_callback("wmb: using MSVC MemoryBarrier()");
    #endif
#elif defined(HAS_GNU_ATOMICS)
    #if defined(HAS_GNU_ATOMIC_BUILTINS)
        /* GCC/Clang: Use __atomic_load_n builtin with acquire semantics
         * __ATOMIC_ACQUIRE ensures proper memory ordering for the load */
        membar_log_callback cb = __atomic_load_n(&log_callback, __ATOMIC_ACQUIRE);
        /* Execute the actual write memory barrier using GCC/Clang builtin BEFORE logging */
        __atomic_thread_fence(__ATOMIC_RELEASE);
        if (cb) cb("wmb: using GNU __atomic_thread_fence(__ATOMIC_RELEASE)");
    #else
        /* Execute the actual write memory barrier using GCC/Clang builtin BEFORE logging */
        __atomic_thread_fence(__ATOMIC_RELEASE);
        /* Fallback for GCC/Clang without atomic support */
        if (log_callback) log_callback("wmb: using GNU __atomic_thread_fence(__ATOMIC_RELEASE)");
    #endif
#elif defined(HAS_BSD_FALLBACK)
    if (log_callback) log_callback("wmb: using BSD __asm__ __volatile__ compiler barrier");
    __asm__ __volatile__("" ::: "memory");
#else
    if (log_callback) log_callback("wmb: using fallback compiler barrier (no hardware barrier)");
    asm volatile ("" ::: "memory");
#endif
}

/**
 * read memory barrier
 * ensures that all memory reads issued before this call are completed before any subsequent reads
 * used to prevent speculative reads from violating program correctness
 *
 * NOTE: If logging is enabled, there is a performance overhead from the callback check
 * and string formatting. Logging should primarily be used for debugging, not in
 * performance-critical production code.
 */
void membar_rmb(void) {
#if defined(HAS_C11_ATOMICS)
    /* C11: Atomically load the callback pointer with acquire semantics */
    membar_log_callback cb = atomic_load_explicit(&log_callback, memory_order_acquire);
    /* Execute the actual read memory barrier BEFORE logging */
    atomic_thread_fence(memory_order_acquire);
    if (cb) cb("rmb: using C11 atomic_thread_fence(memory_order_acquire)");
#elif defined(HAS_MSVC)
    #if defined(HAS_MSVC_ATOMICS)
        /* MSVC: Atomically read callback using compare-exchange trick */
        membar_log_callback cb = (membar_log_callback)_InterlockedCompareExchangePointer(
            (void* volatile*)&log_callback, NULL, NULL);
        /* Execute the actual read memory barrier BEFORE logging */
        MemoryBarrier();
        if (cb) cb("rmb: using MSVC MemoryBarrier()");
    #else
        /* Execute the actual read memory barrier BEFORE logging */
        MemoryBarrier();
        if (log_callback) log_callback("rmb: using MSVC MemoryBarrier()");
    #endif
#elif defined(HAS_GNU_ATOMICS)
    #if defined(HAS_GNU_ATOMIC_BUILTINS)
        /* GCC/Clang: Atomically load callback with acquire semantics */
        membar_log_callback cb = __atomic_load_n(&log_callback, __ATOMIC_ACQUIRE);
        /* Execute the actual read memory barrier BEFORE logging */
        __atomic_thread_fence(__ATOMIC_ACQUIRE);
        if (cb) cb("rmb: using GNU __atomic_thread_fence(__ATOMIC_ACQUIRE)");
    #else
        /* Execute the actual read memory barrier BEFORE logging */
        __atomic_thread_fence(__ATOMIC_ACQUIRE);
        if (log_callback) log_callback("rmb: using GNU __atomic_thread_fence(__ATOMIC_ACQUIRE)");
    #endif
#elif defined(HAS_BSD_FALLBACK)
    if (log_callback) log_callback("rmb: using BSD __asm__ __volatile__ compiler barrier");
    __asm__ __volatile__("" ::: "memory");
#else
    if (log_callback) log_callback("rmb: using fallback compiler barrier (no hardware barrier)");
    asm volatile ("" ::: "memory");
#endif
}

/**
 * full fence (read and write)
 * provides sequential consistency by ensuring all memory operations before this call are completed
 * before any that follow; used to enforce strict ordering across threads
 *
 * NOTE: If logging is enabled, there is a performance overhead from the callback check
 * and string formatting. Logging should primarily be used for debugging, not in
 * performance-critical production code.
 */
void membar_fence(void) {
#if defined(HAS_C11_ATOMICS)
    /* C11: Atomically load the callback pointer with acquire semantics */
    membar_log_callback cb = atomic_load_explicit(&log_callback, memory_order_acquire);
    /* Execute the actual full memory fence with sequential consistency BEFORE logging */
    atomic_thread_fence(memory_order_seq_cst);
    if (cb) cb("fence: using C11 atomic_thread_fence(memory_order_seq_cst)");
#elif defined(HAS_MSVC)
    #if defined(HAS_MSVC_ATOMICS)
        /* MSVC: Atomically read callback using compare-exchange trick */
        membar_log_callback cb = (membar_log_callback)_InterlockedCompareExchangePointer(
            (void* volatile*)&log_callback, NULL, NULL);
        /* Execute the actual full memory fence BEFORE logging */
        MemoryBarrier();
        if (cb) cb("fence: using MSVC MemoryBarrier()");
    #else
        /* Execute the actual full memory fence BEFORE logging */
        MemoryBarrier();
        if (log_callback) log_callback("fence: using MSVC MemoryBarrier()");
    #endif
#elif defined(HAS_GNU_ATOMICS)
    #if defined(HAS_GNU_ATOMIC_BUILTINS)
        /* GCC/Clang: Atomically load callback with acquire semantics */
        membar_log_callback cb = __atomic_load_n(&log_callback, __ATOMIC_ACQUIRE);
        /* Execute the actual full memory fence with sequential consistency BEFORE logging */
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
        if (cb) cb("fence: using GNU __atomic_thread_fence(__ATOMIC_SEQ_CST)");
    #else
        /* Execute the actual full memory fence with sequential consistency BEFORE logging */
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
        if (log_callback) log_callback("fence: using GNU __atomic_thread_fence(__ATOMIC_SEQ_CST)");
    #endif
#elif defined(HAS_BSD_FALLBACK)
    if (log_callback) log_callback("fence: using BSD __asm__ __volatile__ compiler barrier");
    __asm__ __volatile__("" ::: "memory");
#else
    if (log_callback) log_callback("fence: using fallback compiler barrier (no hardware barrier)");
    asm volatile ("" ::: "memory");
#endif
}
