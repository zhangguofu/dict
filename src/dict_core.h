/**
 * @file dict_core.h
 * @brief Dictionary core data structures (internal use)
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
#ifndef __DICT_CORE_H__
#define __DICT_CORE_H__

#include <stdint.h>
#include <stddef.h>
#include "dict.h"


/**
 * @brief Linked list node - inline compact storage
 *
 * Memory layout: [node header][key string+'\0'][value data]
 */
typedef struct dict_node {
    struct dict_node *next;  /* Next node in linked list */
    uint16_t key_len;        /* Key length in bytes */
    uint16_t value_len;      /* Value length in bytes */
    char data[1];            /* Flexible array start, actual size variable */
} dict_node_t;

/* Forward declaration for iterator */
typedef struct dict_iter_impl dict_iter_impl_t;

/**
 * @brief Dictionary structure (internal use)
 */
typedef struct {
    dict_node_t **buckets;    /* Current bucket array (linked list head pointer array) */
    size_t capacity;          /* Current bucket count (power of 2) */
    size_t size;              /* Current element count */
    size_t threshold;         /* Resize threshold (capacity * DICT_LOAD_FACTOR) */
    dict_key_type_t key_type; /* Key type */
    size_t key_size;          /* Key length for NUMBER/BINARY */
    dict_hash_fn_t hash_fn;   /* Custom hash function, NULL uses default */
    dict_iter_impl_t *iter_head;  /* Head of active iterator list */

    /* Migration support for iterator safety */
    dict_node_t **new_buckets;    /* Old bucket array during migration (NULL when not migrating) */
    size_t new_capacity;          /* New capacity during migration */
    int is_migrating;             /* Migration in progress flag */
    int iter_count;               /* Active iterator count */
} dict_t;

/* Node size calculation macro
 * Structure header contains data[1] (1 byte), value storage area needs key_len + value_len
 * sizeof(dict_node_t) already includes data[1], so total size is the sum of both */
#define DICT_NODE_SIZE(key_len, value_len) \
    (sizeof(dict_node_t) + (key_len) + (value_len))

/* Get key pointer */
#define DICT_NODE_KEY(node) ((node)->data)

/* Get value pointer */
#define DICT_NODE_VALUE(node) ((void *)((node)->data + (node)->key_len + 1))

/* Node key data start position (BINARY/NUMBER type compares data directly, no '\0') */
#define DICT_NODE_RAW_KEY(node) ((void *)(node)->data)

/* ============================================
 * Iterator Internal Structure
 * ============================================ */

/**
 * @brief Iterator structure (internal use, opaque to external)
 */
struct dict_iter_impl {
    dict_t *dict;            /* Associated dictionary */
    size_t bucket_idx;       /* Current bucket index (relative to iter_buckets) */
    void *node;             /* Current node pointer */
    dict_node_t **iter_buckets;  /* Snapshot of bucket array at creation time */
    size_t iter_capacity;        /* Snapshot of capacity at creation time */
    dict_iter_impl_t *prev;  /* Previous iterator in list */
    dict_iter_impl_t *next;  /* Next iterator in list */
};

/* ============================================
 * Internal Helper Function Declarations
 * ============================================ */

/**
 * @brief Get key length
 */
size_t dict_get_key_len(const dict_t *d, const void *key);

/**
 * @brief Execute hash calculation
 */
uint32_t dict_do_hash(const dict_t *d, const void *key);

/**
 * @brief Compare two keys
 */
int dict_do_compare(const dict_t *d, const void *k1, const void *k2);

#endif /* __DICT_CORE_H__ */
