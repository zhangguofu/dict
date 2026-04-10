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
    /* Prefer custom hash function */
    if (d->hash_fn) {
        return d->hash_fn(key, dict_get_key_len(d, key));
    }

    /* Otherwise use default hash */
    switch (d->key_type) {
        case DICT_KEY_STRING:
            return murmur3_32(key, DICT_STRLEN((const char *)key));
        case DICT_KEY_NUMBER:
            return dict_number_hash(key, d->key_size);
        case DICT_KEY_BINARY:
        default:
            return murmur3_32(key, d->key_size);
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
            return memcmp(k1, k2, d->key_size);
    }
}

/* ============================================
 * Node Operation Implementation
 * ============================================ */

static dict_node_t *dict_node_create(dict_t *d, const void *key, const void *value, size_t value_len)
{
    size_t key_len;
    size_t total_size;
    dict_node_t *node;
    void *ptr;

    key_len = dict_get_key_len(d, key);

    total_size = DICT_NODE_SIZE(key_len, value_len);

    /* Allocate node */
    node = (dict_node_t *)DICT_MALLOC(total_size);
    if (!node) {
        return NULL;
    }

    /* Set node properties */
    node->next = NULL;
    node->key_len = (uint16_t)key_len;
    node->value_len = (uint16_t)value_len;

    /* Copy key data */
    ptr = DICT_NODE_KEY(node);
    memcpy(ptr, key, key_len);

    /* STRING type needs '\0' terminator */
    if (d->key_type == DICT_KEY_STRING) {
        ((char *)ptr)[key_len] = '\0';
    }

    /* Copy value data */
    if (value && value_len > 0) {
        void *value_ptr = DICT_NODE_VALUE(node);
        memcpy(value_ptr, value, value_len);
    }

    return node;
}

static void dict_node_destroy(dict_node_t *node)
{
    if (node) {
        DICT_FREE(node);
    }
}

static dict_node_t *dict_find_node(dict_t *d, const void *key)
{
    uint32_t hash;
    size_t idx;
    dict_node_t *node;

    hash = DICT_HASH(d, key);
    idx = hash & (d->capacity - 1);

    /* Traverse linked list to find */
    for (node = d->buckets[idx]; node != NULL; node = node->next) {
        if (DICT_COMPARE(d, DICT_NODE_KEY(node), key) == 0) {
            return node;
        }
    }

    return NULL;
}

static int dict_resize_to(dict_t *d, size_t new_capacity)
{
    dict_node_t **old_buckets;
    size_t old_capacity;
    size_t i;
    dict_node_t *node;
    dict_node_t *next;
    size_t idx;

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

    old_buckets = d->buckets;
    old_capacity = d->capacity;

    /* Allocate new bucket array */
    d->buckets = (dict_node_t **)DICT_MALLOC(new_capacity * sizeof(dict_node_t *));
    if (!d->buckets) {
        d->buckets = old_buckets;
        return DICT_ENOMEM;
    }

    /* Initialize new bucket array */
    for (i = 0; i < new_capacity; i++) {
        d->buckets[i] = NULL;
    }

    /* Rehash all nodes */
    for (i = 0; i < old_capacity; i++) {
        node = old_buckets[i];
        while (node) {
            next = node->next;

            /* Recalculate bucket index */
            uint32_t hash = DICT_HASH(d, DICT_NODE_RAW_KEY(node));
            idx = hash & (new_capacity - 1);

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
    dict_t *d;
    size_t i;

    /* Allocate dictionary structure */
    d = (dict_t *)DICT_MALLOC(sizeof(dict_t));
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
    for (i = 0; i < capacity; i++) {
        d->buckets[i] = NULL;
    }

    d->capacity = capacity;
    d->size = 0;
    d->threshold = (size_t)(capacity * DICT_LOAD_FACTOR);

    /* Initialize key type config from config */
    if (config) {
        d->key_type = config->key_type;
        d->key_size = config->key_size;
        d->hash_fn = config->hash_fn;
    } else {
        /* Default config */
        d->key_type = DICT_KEY_STRING;
        d->key_size = 0;
        d->hash_fn = NULL;
    }

    return d;
}

/* ============================================
 * Public Interface Implementation
 * ============================================ */

dict_handle_t dict_create(const dict_config_t *config)
{
    size_t capacity;

    /* Parameter validation */
    if (config) {
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
        if (config->capacity > 0) {
            capacity = config->capacity;
        } else {
            capacity = DICT_DEFAULT_CAPACITY;
        }
    } else {
        /* Default config */
        capacity = DICT_DEFAULT_CAPACITY;
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
    dict_t *d;
    size_t i;

    if (!handle) {
        return DICT_EINVALID;
    }

    d = (dict_t *)handle;

    /* Free all nodes */
    for (i = 0; i < d->capacity; i++) {
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
    dict_t *d;
    dict_node_t *node;
    uint32_t hash;
    size_t idx;
    size_t key_len;

    if (!handle || !key) {
        return DICT_EINVALID;
    }

    d = (dict_t *)handle;

    /* Get key length and check limits */
    key_len = dict_get_key_len(d, key);
    if (key_len > DICT_MAX_LENGTH || value_len > DICT_MAX_LENGTH) {
        return DICT_ETOOLARGE;
    }

    /* Check if key already exists */
    node = dict_find_node(d, key);

    if (node) {
        /* Key exists: check if value size is the same */
        if (node->value_len == value_len) {
            /* Same size, directly overwrite value */
            void *value_ptr = DICT_NODE_VALUE(node);
            if (value && value_len > 0) {
                memcpy(value_ptr, value, value_len);
            }
        } else {
            /* Different size, need to rebuild node */
            dict_node_t **bucket_ptr;
            uint32_t old_hash = DICT_HASH(d, key);
            size_t old_idx = old_hash & (d->capacity - 1);

            /* Remove from old linked list */
            bucket_ptr = &d->buckets[old_idx];
            while (*bucket_ptr != node) {
                bucket_ptr = &(*bucket_ptr)->next;
            }
            *bucket_ptr = node->next;

            /* Free old node */
            dict_node_destroy(node);

            /* Create new node and insert at head of linked list */
            node = dict_node_create(d, key, value, value_len);
            if (!node) {
                return DICT_ENOMEM;
            }

            node->next = d->buckets[old_idx];
            d->buckets[old_idx] = node;
        }
    } else {
        /* Key does not exist: create new node */
        node = dict_node_create(d, key, value, value_len);
        if (!node) {
            return DICT_ENOMEM;
        }

        /* Calculate bucket index */
        hash = DICT_HASH(d, key);
        idx = hash & (d->capacity - 1);

        /* Insert at head of linked list */
        node->next = d->buckets[idx];
        d->buckets[idx] = node;

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

    if (!handle || !key) {
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

    /* Check output buffer */
    if (!value_out) {
        return DICT_EINVALID;
    }

    /* Copy value data */
    if (node->value_len > 0) {
        void *value_ptr = DICT_NODE_VALUE(node);
        memcpy(value_out, value_ptr, node->value_len);
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
    dict_t *d;
    uint32_t hash;
    size_t idx;
    dict_node_t **prev_ptr;
    dict_node_t *node;

    if (!handle || !key) {
        return DICT_EINVALID;
    }

    d = (dict_t *)handle;

    /* Calculate bucket index */
    hash = DICT_HASH(d, key);
    idx = hash & (d->capacity - 1);

    /* Traverse linked list to find */
    prev_ptr = &d->buckets[idx];
    node = d->buckets[idx];

    while (node) {
        if (DICT_COMPARE(d, DICT_NODE_KEY(node), key) == 0) {
            /* Found node, remove from linked list */
            *prev_ptr = node->next;

            /* Free node */
            dict_node_destroy(node);

            /* Update size */
            d->size--;

            return DICT_OK;
        }
        prev_ptr = &node->next;
        node = node->next;
    }

    return DICT_ENOTFOUND;
}

size_t dict_size(dict_handle_t handle)
{
    if (!handle) {
        return 0;
    }
    return ((dict_t *)handle)->size;
}

int dict_clear(dict_handle_t handle)
{
    dict_t *d;
    size_t i;

    if (!handle) {
        return DICT_EINVALID;
    }

    d = (dict_t *)handle;

    /* Free all nodes */
    for (i = 0; i < d->capacity; i++) {
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

    return DICT_OK;
}

int dict_shrink(dict_handle_t handle)
{
    dict_t *d;
    size_t new_capacity;
    double load_factor;

    if (!handle) {
        return DICT_EINVALID;
    }

    d = (dict_t *)handle;

    /* Check if shrink is needed: only shrink when utilization is below 25% */
    if (d->size == 0) {
        /* Empty dictionary shrinks to default capacity */
        new_capacity = DICT_DEFAULT_CAPACITY;
    } else {
        load_factor = (double)d->size / d->capacity;

        /* Utilization above 25%, don't shrink */
        if (load_factor > 0.25) {
            return DICT_OK;
        }

        /* Calculate smallest 2^n capacity that just fits current elements */
        new_capacity = d->size;
        while ((new_capacity & (new_capacity - 1)) != 0) {
            new_capacity++;
        }

        /* Minimum capacity limit */
        if (new_capacity < DICT_DEFAULT_CAPACITY) {
            new_capacity = DICT_DEFAULT_CAPACITY;
        }
    }

    return dict_resize_to(d, new_capacity);
}

int dict_exists(dict_handle_t handle, const void *key)
{
    dict_t *d;

    if (!handle || !key) {
        return 0;
    }

    d = (dict_t *)handle;
    return dict_find_node(d, key) != NULL;
}

size_t dict_capacity(dict_handle_t handle)
{
    if (!handle) {
        return 0;
    }
    return ((dict_t *)handle)->capacity;
}

/* ============================================
 * Iterator Implementation
 * ============================================ */

dict_iter_t dict_iter_create(dict_handle_t handle)
{
    dict_t *d;
    dict_iter_impl_t *iter;

    if (!handle) {
        return NULL;
    }

    d = (dict_t *)handle;

    /* Allocate iterator */
    iter = (dict_iter_impl_t *)DICT_MALLOC(sizeof(dict_iter_impl_t));
    if (!iter) {
        return NULL;
    }

    iter->dict = d;
    iter->bucket_idx = 0;
    iter->node = NULL;

    /* Position at first non-empty bucket */
    for (size_t i = 0; i < d->capacity; i++) {
        if (d->buckets[i] != NULL) {
            iter->bucket_idx = i;
            iter->node = d->buckets[i];
            break;
        }
    }

    return (dict_iter_t)iter;
}

int dict_iter_get(dict_iter_t iter,
                  void *key_out, size_t *klen_out,
                  void *value_out, size_t *vlen_out)
{
    dict_iter_impl_t *it;
    dict_node_t *node;

    if (!iter) {
        return DICT_EINVALID;
    }

    it = (dict_iter_impl_t *)iter;

    /* Current position invalid, traversal complete */
    if (!it->node) {
        return DICT_ENOTFOUND;
    }

    node = (dict_node_t *)it->node;

    /* Copy key data */
    if (key_out && klen_out) {
        memcpy(key_out, DICT_NODE_RAW_KEY(node), node->key_len);
        /* STRING type needs '\0' terminator */
        if (it->dict->key_type == DICT_KEY_STRING) {
            ((char *)key_out)[node->key_len] = '\0';
        }
        *klen_out = node->key_len;
    }

    /* Copy value data */
    if (value_out && vlen_out) {
        memcpy(value_out, DICT_NODE_VALUE(node), node->value_len);
        *vlen_out = node->value_len;
    } else if (vlen_out) {
        *vlen_out = node->value_len;
    }

    return DICT_OK;
}

int dict_iter_next(dict_iter_t iter)
{
    dict_iter_impl_t *it;
    dict_node_t *node;

    if (!iter) {
        return DICT_EINVALID;
    }

    it = (dict_iter_impl_t *)iter;

    /* Current position invalid, return directly */
    if (!it->node) {
        return DICT_ENOTFOUND;
    }

    node = (dict_node_t *)it->node;

    /* Linked list in same bucket has successor */
    if (node->next) {
        it->node = node->next;
        return DICT_OK;
    }

    /* Current bucket linked list finished, find next non-empty bucket */
    for (size_t i = it->bucket_idx + 1; i < it->dict->capacity; i++) {
        if (it->dict->buckets[i] != NULL) {
            it->bucket_idx = i;
            it->node = it->dict->buckets[i];
            return DICT_OK;
        }
    }

    /* No more elements */
    it->node = NULL;
    return DICT_ENOTFOUND;
}

int dict_iter_destroy(dict_iter_t iter)
{
    if (!iter) {
        return DICT_EINVALID;
    }
    DICT_FREE(iter);
    return DICT_OK;
}
