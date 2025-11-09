// membar.c
#include "membar.h"

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
 */
void membar_wmb(void) {
#if defined(HAS_C11_ATOMICS)
    atomic_thread_fence(memory_order_release);
#elif defined(HAS_MSVC)
    MemoryBarrier();
#elif defined(HAS_GNU_ATOMICS)
    __atomic_thread_fence(__ATOMIC_RELEASE);
#elif defined(HAS_BSD_FALLBACK)
    __asm__ __volatile__("" ::: "memory");
#else
    asm volatile ("" ::: "memory");
#endif
}

/**
 * read memory barrier
 * ensures that all memory reads issued before this call are completed before any subsequent reads
 * used to prevent speculative reads from violating program correctness
 */
void membar_rmb(void) {
#if defined(HAS_C11_ATOMICS)
    atomic_thread_fence(memory_order_acquire);
#elif defined(HAS_MSVC)
    MemoryBarrier();
#elif defined(HAS_GNU_ATOMICS)
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
#elif defined(HAS_BSD_FALLBACK)
    __asm__ __volatile__("" ::: "memory");
#else
    asm volatile ("" ::: "memory");
#endif
}

/**
 * full fence (read and write)
 * provides sequential consistency by ensuring all memory operations before this call are completed
 * before any that follow; used to enforce strict ordering across threads
 */
void membar_fence(void) {
#if defined(HAS_C11_ATOMICS)
    atomic_thread_fence(memory_order_seq_cst);
#elif defined(HAS_MSVC)
    MemoryBarrier();
#elif defined(HAS_GNU_ATOMICS)
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
#elif defined(HAS_BSD_FALLBACK)
    __asm__ __volatile__("" ::: "memory");
#else
    asm volatile ("" ::: "memory");
#endif
}
