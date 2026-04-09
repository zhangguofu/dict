/**
 * @file dict_core.h
 * @brief Dictionary core data structures (internal use)
 */
#ifndef __DICT_CORE_H__
#define __DICT_CORE_H__

#include <stdint.h>
#include <stddef.h>
#include "dict.h"

/* Load factor: 0.75 */
#define DICT_LOAD_FACTOR 0.75

/* Default initial capacity */
#define DICT_DEFAULT_CAPACITY 32

/**
 * @brief Linked list node - inline compact storage
 *
 * Memory layout: [node header][key string+'\0'][value data]
 */
typedef struct dict_node {
    struct dict_node *next;  /* Next node in linked list */
    uint16_t key_len;         /* Key length in bytes */
    uint16_t value_len;       /* Value length in bytes */
    char data[1];             /* Flexible array start, actual size variable */
} dict_node_t;

/**
 * @brief Dictionary structure (internal use)
 */
typedef struct {
    dict_node_t **buckets;    /* Bucket array (linked list head pointer array) */
    size_t capacity;          /* Bucket count (power of 2) */
    size_t size;              /* Current element count */
    size_t threshold;         /* Resize threshold (capacity * DICT_LOAD_FACTOR) */
    dict_key_type_t key_type; /* Key type */
    size_t key_size;          /* Key length for NUMBER/BINARY */
    dict_hash_fn_t hash_fn;   /* Custom hash function, NULL uses default */
} dict_t;

/* Node size calculation macro
 * Structure header contains data[1] (1 byte), value storage area needs key_len + value_len
 * sizeof(dict_node_t) already includes data[1], so total size is the sum of both */
#define DICT_NODE_SIZE(key_len, value_len) \
    (sizeof(dict_node_t) + (key_len) + (value_len))

/* Get key pointer */
#define DICT_NODE_KEY(node) ((node)->data)

/* Get value pointer */
#define DICT_NODE_VALUE(node) ((void*)((node)->data + (node)->key_len + 1))

/* Node key data start position (BINARY/NUMBER type compares data directly, no '\0') */
#define DICT_NODE_RAW_KEY(node) ((void*)(node)->data)

/* ============================================
 * Iterator Internal Structure
 * ============================================ */

/**
 * @brief Iterator structure (internal use, opaque to external)
 */
typedef struct {
    dict_t *dict;            /* Associated dictionary */
    size_t bucket_idx;       /* Current bucket index */
    void *node;             /* Current node pointer */
} dict_iter_impl_t;

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
