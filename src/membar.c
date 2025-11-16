// membar.c
#include "membar.h"
#include <stddef.h>

/* optional logging callback */
static membar_log_callback log_callback = NULL;

void membar_set_log_callback(membar_log_callback callback) {
    log_callback = callback;
}

/* prefer c11 atomics if available */
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L) && !defined(__STDC_NO_ATOMICS__)
  #include <stdatomic.h>
  #define HAS_C11_ATOMICS 1
#endif

/* msvc support */
#if defined(_MSC_VER)
  #include <windows.h>
  #define HAS_MSVC 1
#endif

/* gcc/clang fallback */
#if !defined(HAS_C11_ATOMICS) && (defined(__GNUC__) || defined(__clang__))
  #define HAS_GNU_ATOMICS 1
#endif

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
    if (log_callback) log_callback("wmb: using C11 atomic_thread_fence(memory_order_release)");
    atomic_thread_fence(memory_order_release);
#elif defined(HAS_MSVC)
    if (log_callback) log_callback("wmb: using MSVC MemoryBarrier()");
    MemoryBarrier();
#elif defined(HAS_GNU_ATOMICS)
    if (log_callback) log_callback("wmb: using GNU __atomic_thread_fence(__ATOMIC_RELEASE)");
    __atomic_thread_fence(__ATOMIC_RELEASE);
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
    if (log_callback) log_callback("rmb: using C11 atomic_thread_fence(memory_order_acquire)");
    atomic_thread_fence(memory_order_acquire);
#elif defined(HAS_MSVC)
    if (log_callback) log_callback("rmb: using MSVC MemoryBarrier()");
    MemoryBarrier();
#elif defined(HAS_GNU_ATOMICS)
    if (log_callback) log_callback("rmb: using GNU __atomic_thread_fence(__ATOMIC_ACQUIRE)");
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
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
    if (log_callback) log_callback("fence: using C11 atomic_thread_fence(memory_order_seq_cst)");
    atomic_thread_fence(memory_order_seq_cst);
#elif defined(HAS_MSVC)
    if (log_callback) log_callback("fence: using MSVC MemoryBarrier()");
    MemoryBarrier();
#elif defined(HAS_GNU_ATOMICS)
    if (log_callback) log_callback("fence: using GNU __atomic_thread_fence(__ATOMIC_SEQ_CST)");
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
#elif defined(HAS_BSD_FALLBACK)
    if (log_callback) log_callback("fence: using BSD __asm__ __volatile__ compiler barrier");
    __asm__ __volatile__("" ::: "memory");
#else
    if (log_callback) log_callback("fence: using fallback compiler barrier (no hardware barrier)");
    asm volatile ("" ::: "memory");
#endif
}
