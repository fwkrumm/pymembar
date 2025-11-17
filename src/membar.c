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
     * Release ensures all prior writes are visible before the store.
     * This coordinates with the acquire load in the barrier macros (see line 110)
     * to form a synchronizes-with relationship, ensuring thread-safe callback updates. */
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

/*
 * Helper macros to reduce code duplication in barrier functions
 * These macros handle the pattern: load callback atomically -> execute barrier FIRST -> then log
 * The barrier MUST execute before logging to ensure memory ordering takes effect
 * before any callback operations that might access memory. This ensures the memory barrier's
 * ordering guarantees are in effect before the callback potentially accesses memory protected
 * by the barrier.
 */

/* C11 atomics: Load callback atomically, execute barrier BEFORE logging */
#if defined(HAS_C11_ATOMICS)
  #define MEMBAR_WITH_LOG_C11(barrier_call, log_msg) \
    do { \
        membar_log_callback cb = atomic_load_explicit(&log_callback, memory_order_acquire); \
        barrier_call; \
        if (cb) cb(log_msg); \
    } while(0)
#endif

/* MSVC atomics: Load callback with compare-exchange, execute barrier BEFORE logging */
#if defined(HAS_MSVC_ATOMICS)
  #define MEMBAR_WITH_LOG_MSVC(barrier_call, log_msg) \
    do { \
        membar_log_callback cb = (membar_log_callback)_InterlockedCompareExchangePointer( \
            (void* volatile*)&log_callback, NULL, NULL); \
        barrier_call; \
        if (cb) cb(log_msg); \
    } while(0)
#endif

/* GCC/Clang atomics: Load callback atomically, execute barrier BEFORE logging */
#if defined(HAS_GNU_ATOMIC_BUILTINS)
  #define MEMBAR_WITH_LOG_GNU(barrier_call, log_msg) \
    do { \
        membar_log_callback cb = __atomic_load_n(&log_callback, __ATOMIC_ACQUIRE); \
        barrier_call; \
        if (cb) cb(log_msg); \
    } while(0)
#endif

/* Fallback (no atomic support): Execute barrier BEFORE logging */
#define MEMBAR_WITH_LOG_SIMPLE(barrier_call, log_msg) \
  do { \
      barrier_call; \
      if (log_callback) log_callback(log_msg); \
  } while(0)

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
    MEMBAR_WITH_LOG_C11(
        atomic_thread_fence(memory_order_release),
        "wmb: using C11 atomic_thread_fence(memory_order_release)"
    );
#elif defined(HAS_MSVC)
    /* HAS_MSVC_ATOMICS is always defined here (see lines 28-31) since C11 was already handled */
    MEMBAR_WITH_LOG_MSVC(
        MemoryBarrier(),
        "wmb: using MSVC MemoryBarrier()"
    );
#elif defined(HAS_GNU_ATOMICS)
    /* HAS_GNU_ATOMIC_BUILTINS is always defined here (see lines 39-42) since C11 was already handled */
    MEMBAR_WITH_LOG_GNU(
        __atomic_thread_fence(__ATOMIC_RELEASE),
        "wmb: using GNU __atomic_thread_fence(__ATOMIC_RELEASE)"
    );
#elif defined(HAS_BSD_FALLBACK)
    MEMBAR_WITH_LOG_SIMPLE(
        __asm__ __volatile__("" ::: "memory"),
        "wmb: using BSD __asm__ __volatile__ compiler barrier"
    );
#else
    MEMBAR_WITH_LOG_SIMPLE(
        asm volatile ("" ::: "memory"),
        "wmb: using fallback compiler barrier (no hardware barrier)"
    );
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
    MEMBAR_WITH_LOG_C11(
        atomic_thread_fence(memory_order_acquire),
        "rmb: using C11 atomic_thread_fence(memory_order_acquire)"
    );
#elif defined(HAS_MSVC)
    /* HAS_MSVC_ATOMICS is always defined here since C11 was already handled */
    MEMBAR_WITH_LOG_MSVC(
        MemoryBarrier(),
        "rmb: using MSVC MemoryBarrier()"
    );
#elif defined(HAS_GNU_ATOMICS)
    /* HAS_GNU_ATOMIC_BUILTINS is always defined here since C11 was already handled */
    MEMBAR_WITH_LOG_GNU(
        __atomic_thread_fence(__ATOMIC_ACQUIRE),
        "rmb: using GNU __atomic_thread_fence(__ATOMIC_ACQUIRE)"
    );
#elif defined(HAS_BSD_FALLBACK)
    MEMBAR_WITH_LOG_SIMPLE(
        __asm__ __volatile__("" ::: "memory"),
        "rmb: using BSD __asm__ __volatile__ compiler barrier"
    );
#else
    MEMBAR_WITH_LOG_SIMPLE(
        asm volatile ("" ::: "memory"),
        "rmb: using fallback compiler barrier (no hardware barrier)"
    );
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
    MEMBAR_WITH_LOG_C11(
        atomic_thread_fence(memory_order_seq_cst),
        "fence: using C11 atomic_thread_fence(memory_order_seq_cst)"
    );
#elif defined(HAS_MSVC)
    /* HAS_MSVC_ATOMICS is always defined here since C11 was already handled */
    MEMBAR_WITH_LOG_MSVC(
        MemoryBarrier(),
        "fence: using MSVC MemoryBarrier()"
    );
#elif defined(HAS_GNU_ATOMICS)
    /* HAS_GNU_ATOMIC_BUILTINS is always defined here since C11 was already handled */
    MEMBAR_WITH_LOG_GNU(
        __atomic_thread_fence(__ATOMIC_SEQ_CST),
        "fence: using GNU __atomic_thread_fence(__ATOMIC_SEQ_CST)"
    );
#elif defined(HAS_BSD_FALLBACK)
    MEMBAR_WITH_LOG_SIMPLE(
        __asm__ __volatile__("" ::: "memory"),
        "fence: using BSD __asm__ __volatile__ compiler barrier"
    );
#else
    MEMBAR_WITH_LOG_SIMPLE(
        asm volatile ("" ::: "memory"),
        "fence: using fallback compiler barrier (no hardware barrier)"
    );
#endif
}
