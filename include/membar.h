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

/**
 * logging callback function type
 * @param message - the log message to output
 */
typedef void (*membar_log_callback)(const char* message);

/**
 * set optional logging callback
 * @param callback - function to call for logging, or NULL to disable logging
 */
MEMBAR_EXPORT void membar_set_log_callback(membar_log_callback callback);

#ifdef __cplusplus
}
#endif
