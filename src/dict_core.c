/**
 * @file dict_core.c
 * @brief Dictionary core implementation
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
#include "dict_core.h"
#include "dict_hash.h"

#include <string.h>

/* ============================================
 * Convenience Macros
 * ============================================ */

#ifndef DICT_DEFAULT_CAPACITY
#define DICT_DEFAULT_CAPACITY 32
#endif

/* Load factor: DICT_LOAD_FACTOR_X100 / 100.0 */
#ifndef DICT_LOAD_FACTOR_X100
    #define DICT_LOAD_FACTOR_X100 75
#endif
#define DICT_LOAD_FACTOR (((float)DICT_LOAD_FACTOR_X100) / 100.0f)


#define DICT_HASH(d, key) (dict_do_hash((d), (key)))
#define DICT_COMPARE(d, k1, k2) (dict_do_compare((d), (k1), (k2)))


/* ============================================
 * Internal Function Declarations
 * ============================================ */

/**
 * @brief Internal create dictionary (allocate memory but don't zero)
 */
static dict_t *dict_create_internal(const dict_config_t *config, size_t capacity);

/**
 * @brief Allocate and initialize node
 */
static dict_node_t *dict_node_create(dict_t *d, const void *key, const void *value, size_t value_len);

/**
 * @brief Free node
 */
static void dict_node_destroy(dict_node_t *node);

/**
 * @brief Find node corresponding to key
 * @return Node pointer if found, NULL if not found
 */
static dict_node_t *dict_find_node(dict_t *d, const void *key);

/**
 * @brief Resize dictionary capacity
 * @param d Dictionary
 * @param new_capacity New capacity (must be power of 2)
 */
static int dict_resize_to(dict_t *d, size_t new_capacity);

/**
 * @brief Clone a node (deep copy)
 * @param d Dictionary (for key_type info)
 * @param src Source node to clone
 * @return New cloned node, or NULL if allocation failed
 */
static dict_node_t *dict_node_clone(const dict_t *d, const dict_node_t *src);

/**
 * @brief Free all nodes and bucket array in a snapshot
 * @param snap_buckets Snapshot bucket array
 * @param snap_capacity Snapshot capacity
 */
static void dict_iter_snapshot_free(dict_node_t **snap_buckets, size_t snap_capacity);

/* ============================================
 * Internal Helper Function Implementation
 * ============================================ */

size_t dict_get_key_len(const dict_t *d, const void *key)
{
    switch (d->key_type) {
        case DICT_KEY_STRING:
            return DICT_STRLEN((const char *)key);
        case DICT_KEY_NUMBER:
        case DICT_KEY_BINARY:
        default:
            return d->key_size;
    }
}

uint32_t dict_do_hash(const dict_t *d, const void *key)
{
    if (NULL == d->hash_fn) {
        // use built-in hash function
        switch (d->key_type) {
            case DICT_KEY_STRING:
                return murmur3_32(key, DICT_STRLEN((const char *)key));
            case DICT_KEY_NUMBER:
                return dict_number_hash(key, d->key_size);
            case DICT_KEY_BINARY:
            default:
                return murmur3_32(key, d->key_size);
        }
    } else {
        // use custom hash function
        return d->hash_fn(key, dict_get_key_len(d, key));
    }
}

int dict_do_compare(const dict_t *d, const void *k1, const void *k2)
{
    switch (d->key_type) {
        case DICT_KEY_STRING:
            return DICT_STRCMP((const char *)k1, (const char *)k2);
        case DICT_KEY_NUMBER:
        case DICT_KEY_BINARY:
        default:
            return DICT_MEMCMP(k1, k2, d->key_size);
    }
}

/* ============================================
 * Node Operation Implementation
 * ============================================ */

static dict_node_t *dict_node_create(dict_t *d, const void *key, const void *value, size_t value_len)
{
    size_t key_len = dict_get_key_len(d, key);
    size_t total_size = DICT_NODE_SIZE(key_len, value_len);

    /* Allocate node */
    dict_node_t *node = (dict_node_t *)DICT_MALLOC(total_size);
    if (!node) {
        return NULL;
    }

    /* Set node properties */
    node->next = NULL;
    node->key_len = (uint16_t)key_len;
    node->value_len = (uint16_t)value_len;

    /* Copy key data */
    void *ptr = DICT_NODE_KEY(node);
    DICT_MEMCPY(ptr, key, key_len);

    /* STRING type needs '\0' terminator */
    if (d->key_type == DICT_KEY_STRING) {
        ((char *)ptr)[key_len] = '\0';
    }

    /* Copy value data */
    if (value && value_len > 0) {
        void *value_ptr = DICT_NODE_VALUE(node);
        DICT_MEMCPY(value_ptr, value, value_len);
    }

    return node;
}

static void dict_node_destroy(dict_node_t *node)
{
    if (node) {
        DICT_FREE(node);
    }
}

static dict_node_t *dict_node_clone(const dict_t *d, const dict_node_t *src)
{
    if (!src) {
        return NULL;
    }

    size_t total_size = DICT_NODE_SIZE(src->key_len, src->value_len);

    /* Allocate new node */
    dict_node_t *node = (dict_node_t *)DICT_MALLOC(total_size);
    if (!node) {
        return NULL;
    }

    /* Copy node properties */
    node->next = NULL;  /* Will be set by caller */
    node->key_len = src->key_len;
    node->value_len = src->value_len;

    /* Copy key data */
    void *ptr = DICT_NODE_KEY(node);
    DICT_MEMCPY(ptr, src->data, src->key_len);

    /* STRING type needs '\0' terminator */
    if (d->key_type == DICT_KEY_STRING) {
        ((char *)ptr)[src->key_len] = '\0';
    }

    /* Copy value data */
    if (src->value_len > 0) {
        void *value_ptr = DICT_NODE_VALUE(node);
        DICT_MEMCPY(value_ptr, DICT_NODE_VALUE(src), src->value_len);
    }

    return node;
}

static void dict_iter_snapshot_free(dict_node_t **snap_buckets, size_t snap_capacity)
{
    if (!snap_buckets) {
        return;
    }

    /* Free all nodes in each bucket's linked list */
    for (size_t i = 0; i < snap_capacity; i++) {
        dict_node_t *node = snap_buckets[i];
        while (node) {
            dict_node_t *next = node->next;
            dict_node_destroy(node);
            node = next;
        }
    }

    /* Free bucket array itself */
    DICT_FREE(snap_buckets);
}

static dict_node_t *dict_find_node(dict_t *d, const void *key)
{
    uint32_t hash = DICT_HASH(d, key);
    size_t idx = hash & (d->capacity - 1);

    /* Traverse linked list to find */
    for (dict_node_t *node = d->buckets[idx]; node != NULL; node = node->next) {
        if (DICT_COMPARE(d, DICT_NODE_KEY(node), key) == 0) {
            return node;
        }
    }

    return NULL;
}

static int dict_resize_to(dict_t *d, size_t new_capacity)
{
    /* Cannot shrink below minimum capacity */
    if (new_capacity < DICT_DEFAULT_CAPACITY) {
        new_capacity = DICT_DEFAULT_CAPACITY;
    }

    /* Cannot shrink below element count */
    if (new_capacity < d->size) {
        new_capacity = d->size;
        /* Round up to power of 2 */
        while ((new_capacity & (new_capacity - 1)) != 0) {
            new_capacity++;
        }
    }

    /* Capacity unchanged, no need to resize */
    if (new_capacity == d->capacity) {
        return DICT_OK;
    }

    dict_node_t **old_buckets = d->buckets;
    size_t old_capacity = d->capacity;

    /* Allocate new bucket array */
    d->buckets = (dict_node_t **)DICT_MALLOC(new_capacity * sizeof(dict_node_t *));
    if (!d->buckets) {
        d->buckets = old_buckets;
        return DICT_ENOMEM;
    }

    /* Initialize new bucket array */
    for (size_t i = 0; i < new_capacity; i++) {
        d->buckets[i] = NULL;
    }

    /* Rehash all nodes */
    for (size_t i = 0; i < old_capacity; i++) {
        dict_node_t *node = old_buckets[i];
        while (node) {
            dict_node_t *next = node->next;

            /* Recalculate bucket index */
            uint32_t hash = DICT_HASH(d, DICT_NODE_RAW_KEY(node));
            size_t idx = hash & (new_capacity - 1);

            /* Insert into new bucket */
            node->next = d->buckets[idx];
            d->buckets[idx] = node;

            node = next;
        }
    }

    /* Update dictionary properties */
    d->capacity = new_capacity;
    d->threshold = (size_t)(new_capacity * DICT_LOAD_FACTOR);

    /* Free old bucket array */
    DICT_FREE(old_buckets);

    return DICT_OK;
}

static dict_t *dict_create_internal(const dict_config_t *config, size_t capacity)
{
    /* Allocate dictionary structure */
    dict_t *d = (dict_t *)DICT_MALLOC(sizeof(dict_t));
    if (!d) {
        return NULL;
    }

    /* Allocate bucket array */
    d->buckets = (dict_node_t **)DICT_MALLOC(capacity * sizeof(dict_node_t *));
    if (!d->buckets) {
        DICT_FREE(d);
        return NULL;
    }

    /* Initialize bucket array */
    for (size_t i = 0; i < capacity; i++) {
        d->buckets[i] = NULL;
    }

    d->key_type = config->key_type;
    d->key_size = config->key_size;
    d->hash_fn = config->hash_fn;
    d->capacity = capacity;
    d->size = 0;
    d->threshold = (size_t)(capacity * DICT_LOAD_FACTOR);

    return d;
}

/* ============================================
 * Public Interface Implementation
 * ============================================ */

dict_handle_t dict_create(const dict_config_t *config)
{
    if (NULL == config) {
        return NULL;
    }

    /* Validate key_type */
    if (config->key_type < DICT_KEY_STRING || config->key_type > DICT_KEY_BINARY) {
        return NULL;
    }

    /* NUMBER/BINARY must specify key_size */
    if (config->key_type != DICT_KEY_STRING && config->key_size == 0) {
        return NULL;
    }

    /* Check key_size limit for NUMBER/BINARY types */
    if (config->key_type != DICT_KEY_STRING && config->key_size > DICT_MAX_LENGTH) {
        return NULL;
    }

    /* Get capacity */
    size_t capacity = DICT_DEFAULT_CAPACITY;
    if (config->capacity > 0) {
        capacity = config->capacity;
    }

    /* Ensure capacity is power of 2 */
    if ((capacity & (capacity - 1)) != 0) {
        size_t p = 1;
        while (p < capacity) {
            p <<= 1;
        }
        capacity = p;
    }

    return (dict_handle_t)dict_create_internal(config, capacity);
}

int dict_destroy(dict_handle_t handle)
{
    if (!handle) {
        return DICT_EINVALID;
    }

    dict_t *d = (dict_t *)handle;

    /* Free all nodes in dictionary */
    for (size_t i = 0; i < d->capacity; i++) {
        dict_node_t *node = d->buckets[i];
        while (node) {
            dict_node_t *next = node->next;
            dict_node_destroy(node);
            node = next;
        }
    }

    /* Free bucket array */
    DICT_FREE(d->buckets);

    /* Free dictionary structure */
    DICT_FREE(d);
    return DICT_OK;
}

int dict_set(dict_handle_t handle, const void *key, const void *value, size_t value_len)
{
    if (NULL == handle || NULL == key) {
        return DICT_EINVALID;
    }

    dict_t * d = (dict_t *)handle;

    /* Get key length and check limits */
    size_t key_len = dict_get_key_len(d, key);
    if (key_len > DICT_MAX_LENGTH || value_len > DICT_MAX_LENGTH) {
        return DICT_ETOOLARGE;
    }

    /* Check if key already exists */
    dict_node_t *node = dict_find_node(d, key);
    if (node) {
        /* Key exists: check if value size is the same */
        if (node->value_len == value_len) {
            /* Same size, directly overwrite value */
            void *value_ptr = DICT_NODE_VALUE(node);
            if (value && value_len > 0) {
                DICT_MEMCPY(value_ptr, value, value_len);
            }
        } else {
            /* Different size create new node first */
            dict_node_t *new_node = dict_node_create(d, key, value, value_len);
            if (NULL == new_node) {
                return DICT_ENOMEM;
            }

            uint32_t hash = DICT_HASH(d, key);
            size_t index = hash & (d->capacity - 1);

            /* Remove from linked list */
            dict_node_t **bucket_ptr = &d->buckets[index];
            while (*bucket_ptr != node) {
                bucket_ptr = &(*bucket_ptr)->next;
            }
            // Do remove
            new_node->next = node->next;
            *bucket_ptr = new_node;

            /* Free old node */
            dict_node_destroy(node);
        }
    } else {
        /* Key does not exist: create new node */
        dict_node_t *new_node = dict_node_create(d, key, value, value_len);
        if (NULL == new_node) {
            return DICT_ENOMEM;
        }

        /* Calculate bucket index */
        uint32_t hash = DICT_HASH(d, key);
        size_t index = hash & (d->capacity - 1);

        /* Insert at head of linked list */
        new_node->next = d->buckets[index];
        d->buckets[index] = new_node;

        /* Update size */
        d->size++;

        /* Check if resize is needed */
        if (d->size >= d->threshold) {
            int ret = dict_resize_to(d, d->capacity * 2);
            if (ret != DICT_OK) {
                return ret;
            }
        }
    }

    return DICT_OK;
}

int dict_get(dict_handle_t handle, const void *key, void *value_out, size_t buf_len)
{
    dict_t *d;
    dict_node_t *node;

    if (NULL == handle || NULL == key || NULL == value_out) {
        return DICT_EINVALID;
    }

    d = (dict_t *)handle;

    /* Find node */
    node = dict_find_node(d, key);
    if (!node) {
        return DICT_ENOTFOUND;
    }

    /* Check buffer size */
    if (buf_len < node->value_len) {
        return DICT_ETOOSMALL;
    }

    /* Copy value data */
    if (node->value_len > 0) {
        void *value_ptr = DICT_NODE_VALUE(node);
        DICT_MEMCPY(value_out, value_ptr, node->value_len);
    }

    return DICT_OK;
}

int dict_get_size(dict_handle_t handle, const void *key, size_t *size_out)
{
    dict_t *d;
    dict_node_t *node;

    if (!handle || !key || !size_out) {
        return DICT_EINVALID;
    }

    d = (dict_t *)handle;

    /* Find node */
    node = dict_find_node(d, key);
    if (!node) {
        return DICT_ENOTFOUND;
    }

    *size_out = node->value_len;

    return DICT_OK;
}

int dict_delete(dict_handle_t handle, const void *key)
{
    if (NULL == handle || NULL == key) {
        return DICT_EINVALID;
    }

    dict_t *d = (dict_t *)handle;

    /* Calculate bucket index */
    uint32_t hash = DICT_HASH(d, key);
    size_t idx = hash & (d->capacity - 1);

    /* Traverse linked list to find */
    dict_node_t **prev_ptr = &d->buckets[idx];
    dict_node_t *node = d->buckets[idx];

    while (node) {
        if (DICT_COMPARE(d, DICT_NODE_KEY(node), key) == 0) {
            /* Found node, remove from linked list */
            *prev_ptr = node->next;

            /* Free node */
            dict_node_destroy(node);

            /* Update size */
            d->size--;

            /* Note: In snapshot strategy, iterator invalidation is NOT needed.
             * Iterators hold independent snapshots and are unaffected by dict modifications. */

            return DICT_OK;
        }
        prev_ptr = &node->next;
        node = node->next;
    }

    return DICT_ENOTFOUND;
}

size_t dict_size(dict_handle_t handle)
{
    if (NULL == handle) {
        return 0;
    }
    return ((dict_t *)handle)->size;
}

int dict_clear(dict_handle_t handle)
{
    if (NULL == handle) {
        return DICT_EINVALID;
    }

    dict_t *d = (dict_t *)handle;

    /* Free all nodes */
    for (size_t i = 0; i < d->capacity; i++) {
        dict_node_t *node = d->buckets[i];
        while (node) {
            dict_node_t *next = node->next;
            dict_node_destroy(node);
            node = next;
        }
        d->buckets[i] = NULL;
    }

    /* Reset size */
    d->size = 0;

    /* Note: In snapshot strategy, iterator invalidation is NOT needed.
     * Iterators hold independent snapshots and are unaffected by dict modifications. */

    return DICT_OK;
}

int dict_shrink(dict_handle_t handle)
{
    if (NULL == handle) {
        return DICT_EINVALID;
    }

    dict_t *d = (dict_t *)handle;

    /* Check if shrink is needed: only shrink when utilization is below 25% */
    if (d->size == 0) {
        /* Empty dictionary shrinks to default capacity */
        return dict_resize_to(d, DICT_DEFAULT_CAPACITY);
    }

    double load_factor = (double)d->size / d->capacity;

    /* Utilization above 25%, don't shrink */
    if (load_factor > 0.25) {
        return DICT_OK;
    }

    /* Calculate smallest 2^n capacity that just fits current elements */
    size_t new_capacity = d->size;
    while ((new_capacity & (new_capacity - 1)) != 0) {
        new_capacity++;
    }

    /* Minimum capacity limit */
    if (new_capacity < DICT_DEFAULT_CAPACITY) {
        new_capacity = DICT_DEFAULT_CAPACITY;
    }

    return dict_resize_to(d, new_capacity);
}

int dict_exists(dict_handle_t handle, const void *key)
{
    if (NULL == handle || NULL == key) {
        return 0;
    }

    dict_t *d = (dict_t *)handle;
    return dict_find_node(d, key) != NULL;
}

size_t dict_capacity(dict_handle_t handle)
{
    if (NULL == handle) {
        return 0;
    }
    return ((dict_t *)handle)->capacity;
}

/* ============================================
 * Iterator Implementation
 * ============================================ */

dict_iter_t dict_iter_create(dict_handle_t handle)
{
    if (NULL == handle) {
        return NULL;
    }

    dict_t *d = (dict_t *)handle;

    /* Allocate iterator */
    dict_iter_impl_t *iter = (dict_iter_impl_t *)DICT_MALLOC(sizeof(dict_iter_impl_t));
    if (NULL == iter) {
        return NULL;
    }

    iter->key_type = d->key_type;  /* Cache key_type for use after dict_destroy */
    iter->bucket_idx = 0;
    iter->node = NULL;
    iter->snap_buckets = NULL;
    iter->snap_capacity = d->capacity;

    /* Allocate snapshot bucket array */
    iter->snap_buckets = (dict_node_t **)DICT_MALLOC(d->capacity * sizeof(dict_node_t *));
    if (NULL == iter->snap_buckets) {
        DICT_FREE(iter);
        return NULL;
    }

    /* Initialize snapshot bucket array to NULL */
    for (size_t i = 0; i < d->capacity; i++) {
        iter->snap_buckets[i] = NULL;
    }

    /* Deep copy all nodes from dictionary to snapshot */
    for (size_t i = 0; i < d->capacity; i++) {
        dict_node_t *src_node = d->buckets[i];
        dict_node_t *prev_clone = NULL;

        while (src_node) {
            /* Clone current node */
            dict_node_t *clone_node = dict_node_clone(d, src_node);
            if (NULL == clone_node) {
                /* Allocation failed, free partially constructed snapshot */
                dict_iter_snapshot_free(iter->snap_buckets, d->capacity);
                DICT_FREE(iter);
                return NULL;
            }

            /* Link to previous cloned node in this bucket's list */
            if (prev_clone) {
                prev_clone->next = clone_node;
            } else {
                /* First node in this bucket */
                iter->snap_buckets[i] = clone_node;
            }
            prev_clone = clone_node;

            src_node = src_node->next;
        }
    }

    /* Position at first non-empty bucket in snapshot */
    for (size_t i = 0; i < iter->snap_capacity; i++) {
        if (iter->snap_buckets[i] != NULL) {
            iter->bucket_idx = i;
            iter->node = iter->snap_buckets[i];
            break;
        }
    }

    return (dict_iter_t)iter;
}

int dict_iter_get(dict_iter_t iter,
                  void *key_out, size_t *klen_out,
                  void *value_out, size_t *vlen_out)
{
    if (NULL == iter) {
        return DICT_EINVALID;
    }

    dict_iter_impl_t *it = (dict_iter_impl_t *)iter;

    /* Current position invalid, traversal complete */
    if (!it->node) {
        return DICT_ENOTFOUND;
    }

    dict_node_t *node = (dict_node_t *)it->node;

    /* Copy key data */
    if (key_out) {
        DICT_MEMCPY(key_out, DICT_NODE_RAW_KEY(node), node->key_len);
        /* STRING type needs '\0' terminator */
        if (it->key_type == DICT_KEY_STRING) {
            ((char *)key_out)[node->key_len] = '\0';
        }
    }

    if (klen_out) {
        *klen_out = node->key_len;
    }

    /* Copy value data */
    if (value_out && vlen_out) {
        DICT_MEMCPY(value_out, DICT_NODE_VALUE(node), node->value_len);
        *vlen_out = node->value_len;
    } else if (vlen_out) {
        *vlen_out = node->value_len;
    }

    return DICT_OK;
}

int dict_iter_is_valid(dict_iter_t iter)
{
    if (NULL == iter) {
        return 0;
    }

    dict_iter_impl_t *it = (dict_iter_impl_t *)iter;

    /* Check if current node is valid (pointing to a valid element) */
    return (it->node != NULL) ? 1 : 0;
}

int dict_iter_next(dict_iter_t iter)
{
    if (NULL == iter) {
        return DICT_EINVALID;
    }

    dict_iter_impl_t *it = (dict_iter_impl_t *)iter;

    /* Current position invalid, return directly */
    if (NULL == it->node) {
        return DICT_ENOTFOUND;
    }

    dict_node_t *node = (dict_node_t *)it->node;

    /* Linked list in same bucket has successor */
    if (node->next) {
        it->node = node->next;
        return DICT_OK;
    }

    /* Current bucket linked list finished, find next non-empty bucket in snapshot */
    for (size_t i = it->bucket_idx + 1; i < it->snap_capacity; i++) {
        if (it->snap_buckets[i] != NULL) {
            it->bucket_idx = i;
            it->node = it->snap_buckets[i];
            return DICT_OK;
        }
    }

    /* No more elements */
    it->node = NULL;
    return DICT_ENOTFOUND;
}

int dict_iter_destroy(dict_iter_t iter)
{
    if (NULL == iter) {
        return DICT_EINVALID;
    }

    dict_iter_impl_t *it = (dict_iter_impl_t *)iter;

    /* Free snapshot resources */
    dict_iter_snapshot_free(it->snap_buckets, it->snap_capacity);

    DICT_FREE(iter);
    return DICT_OK;
}
