// membar.h
#pragma once

#ifdef _WIN32
    #define MEMBAR_EXPORT __declspec(dllexport)
#else
    #define MEMBAR_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * write memory barrier
 * ensures that all previous memory writes are visible before any subsequent writes
 */
MEMBAR_EXPORT void membar_wmb(void);

/**
 * read memory barrier
 * ensures that all previous memory reads are completed before any subsequent reads
 */
MEMBAR_EXPORT void membar_rmb(void);

/**
 * full fence (both read and write)
 * provides sequential consistency across memory operations
 */
MEMBAR_EXPORT void membar_fence(void);

#ifdef __cplusplus
}
#endif
