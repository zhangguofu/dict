/**
 * @file dict.h
 * @brief General-purpose dictionary module - C language key-value storage
 *
 * Supports arbitrary data type storage with copy mode, dictionary manages memory automatically.
 * Dual platform support: designed for RT-Thread, code also runs on PC.
 *
 * @copyright Copyright (c) 2020-2026 Gary Zhang [cleancode@163.com]
 *
 * @license The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */
#ifndef __DICT_H__
#define __DICT_H__

#include <stddef.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/* ============================================
 * Platform Abstraction Layer
 * ============================================ */

#if !defined(DICT_RUN_ON_PC)
    #include <rtthread.h>

    #define DICT_MALLOC(size)     rt_malloc(size)
    #define DICT_FREE(ptr)        rt_free(ptr)
    #define DICT_STRCMP(a, b)     rt_strcmp(a, b)
    #define DICT_STRLEN(str)      rt_strlen(str)
#else
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>

    #define DICT_MALLOC(size)     malloc(size)
    #define DICT_FREE(ptr)        free(ptr)
    #define DICT_STRCMP(a, b)     strcmp(a, b)
    #define DICT_STRLEN(str)      strlen(str)
#endif

/* ============================================
 * Error Codes
 * ============================================ */

#define DICT_OK          0   /* Success */
#define DICT_EINVALID   -1   /* Invalid parameter (NULL pointer or invalid argument) */
#define DICT_ENOMEM     -2   /* Memory allocation failed */
#define DICT_ENOTFOUND  -3   /* Key not found */
#define DICT_ETOOSMALL  -4   /* Buffer too small */
#define DICT_ETOOLARGE  -5   /* Key or value length too large (exceeds DICT_MAX_LENGTH bytes) */

/* Maximum length for keys and values (limited by uint16_t storage) */
#define DICT_MAX_LENGTH    UINT16_MAX   /* 65535 */

/* ============================================
 * Type Definitions
 * ============================================ */

/**
 * @brief Key type enumeration
 */
typedef enum {
    DICT_KEY_STRING,    /* Variable-length string (default, uses strlen internally) */
    DICT_KEY_NUMBER,    /* Integer type (fixed-length, optimized hash) */
    DICT_KEY_BINARY,    /* Fixed-length binary data (fixed-length, uses MurmurHash3) */
} dict_key_type_t;

/**
 * @brief Hash function type
 * @param key Key pointer
 * @param len Key length
 * @return 32-bit hash value
 */
typedef uint32_t (*dict_hash_fn_t)(const void *key, size_t len);

/**
 * @brief Dictionary handle (opaque type for user)
 */
typedef void *dict_handle_t;

/**
 * @brief Iterator type (opaque handle)
 */
typedef void *dict_iter_t;

/**
 * @brief Dictionary configuration structure
 */
typedef struct {
    size_t capacity;              /* Initial capacity (recommended 16/32/64), default 32 */
    dict_key_type_t key_type;     /* Key type (STRING/NUMBER/BINARY) */
    size_t key_size;              /* Key length for NUMBER/BINARY type, ignored for STRING */
    dict_hash_fn_t hash_fn;       /* Optional custom hash function, NULL uses default */
} dict_config_t;

/* Default configuration (from rtconfig.h or fallback) */
#ifndef DICT_DEFAULT_CAPACITY
    #define DICT_DEFAULT_CAPACITY 32
#endif

/* ============================================
 * Interface Function Declarations
 * ============================================ */

/**
 * @brief Create a dictionary
 * @param config Configuration parameters, uses default when NULL
 * @return Returns the dictionary handle on success, NULL otherwise
 */
dict_handle_t dict_create(const dict_config_t *config);

/**
 * @brief Set key-value pair
 *        - If key does not exist: insert new node
 *        - If key exists: free old value, copy new value
 * @param handle Dictionary handle
 * @param key Key pointer (type determined by key_type at creation)
 * @param value Value pointer
 * @param value_len Value length in bytes
 * @return Returns DICT_OK (0) on success, error code (<0) otherwise
 */
int dict_set(dict_handle_t handle, const void *key, const void *value, size_t value_len);

/**
 * @brief Get value
 * @param handle Dictionary handle
 * @param key Key pointer
 * @param value_out Value output buffer
 * @param buf_len Buffer size
 * @return Returns DICT_OK (0) on success, error code (<0) otherwise
 */
int dict_get(dict_handle_t handle, const void *key, void *value_out, size_t buf_len);

/**
 * @brief Get value size
 * @param handle Dictionary handle
 * @param key Key pointer
 * @param size_out Size output pointer
 * @return Returns DICT_OK (0) on success, error code (<0) otherwise
 */
int dict_get_size(dict_handle_t handle, const void *key, size_t *size_out);

/**
 * @brief Delete key-value pair
 * @param handle Dictionary handle
 * @param key Key pointer
 * @return Returns DICT_OK (0) on success, error code (<0) otherwise
 */
int dict_delete(dict_handle_t handle, const void *key);

/**
 * @brief Get element count
 * @param handle Dictionary handle
 * @return Element count
 */
size_t dict_size(dict_handle_t handle);

/**
 * @brief Clear all key-value pairs
 * @param handle Dictionary handle
 * @return Returns DICT_OK (0) on success
 */
int dict_clear(dict_handle_t handle);

/**
 * @brief Shrink dictionary capacity
 *
 * When element count is below 25% of capacity, automatically shrink to the smallest
 * 2^n capacity that can hold current elements, to release excess memory. Minimum capacity is 32.
 *
 * @note If utilization is above 25%, this function does nothing
 * @param handle Dictionary handle
 * @return Returns DICT_OK (0) on success, error code (<0) otherwise
 */
int dict_shrink(dict_handle_t handle);

/**
 * @brief Check if key exists
 * @param handle Dictionary handle
 * @param key Key pointer
 * @return 1 if exists, 0 if not exists
 */
int dict_exists(dict_handle_t handle, const void *key);

/**
 * @brief Get current dictionary capacity
 * @param handle Dictionary handle
 * @return Bucket array capacity
 */
size_t dict_capacity(dict_handle_t handle);

/**
 * @brief Create iterator
 * @param handle Dictionary handle
 * @return Returns the iterator handle on success, NULL otherwise
 *
 * @note After creation, iterator points to first element (if dictionary is not empty)
 */
dict_iter_t dict_iter_create(dict_handle_t handle);

/**
 * @brief Get current element data
 *
 * @param iter Iterator handle
 * @param key_out Key output buffer (can be NULL)
 * @param klen_out Key length output (can be NULL)
 * @param value_out Value output buffer (can be NULL)
 * @param vlen_out Value length output (can be NULL)
 * @return Returns DICT_OK (0) on success, error code (<0) otherwise
 */
int dict_iter_get(dict_iter_t iter,
                  void *key_out, size_t *klen_out,
                  void *value_out, size_t *vlen_out);

/**
 * @brief Check if iterator current position is valid
 * @param iter Iterator handle
 * @return 1 if valid (pointing to a valid element), 0 if invalid (traversal complete)
 */
int dict_iter_is_valid(dict_iter_t iter);

/**
 * @brief Move to next element
 *
 * Only updates iterator position, does not return data
 *
 * @param iter Iterator handle
 * @return Returns DICT_OK (0) on success, error code (<0) otherwise
 */
int dict_iter_next(dict_iter_t iter);

/**
 * @brief Destroy iterator
 * @param iter Iterator handle
 * @return Returns DICT_OK (0) on success, error code (<0) otherwise
 *
 * @note Must destroy all associated iterators before destroying dictionary
 */
int dict_iter_destroy(dict_iter_t iter);

/**
 * @brief Destroy dictionary, free all memory
 * @param handle Dictionary handle
 * @return Returns DICT_OK (0) on success, error code (<0) otherwise
 */
int dict_destroy(dict_handle_t handle);


#ifdef __cplusplus
}
#endif

#endif /* __DICT_H__ */
