/**
 * @file dict_hash.c
 * @brief Hash algorithm implementation
 *
 * Supports:
 * - MurmurHash3 32-bit: suitable for strings and binary data
 * - Integer hash: optimized implementation for 1/2/4/8 byte integers
 *
 * Reference: https://github.com/aappleby/smhasher
 * Features: pure bit operations, no lookup tables, ARM32 friendly, small code size
 *
 * Note: ARM32 does not support unaligned memory access, so memcpy is used to safely read 4-byte blocks
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
#include "dict_hash.h"
#include <string.h>

/**
 * @brief Rotate left
 */
#define ROTL32(x, r) (((x) << (r)) | ((x) >> (32 - (r))))

/**
 * @brief Safely get 32-bit data (little-endian)
 *
 * Uses memcpy to avoid unaligned access issues on ARM32
 */
static inline uint32_t GETBLOCK32(const uint8_t *p, size_t i)
{
    uint32_t val;
    DICT_HASH_MEMCPY(&val, p + i * 4, sizeof(val));
    return val;
}

/**
 * @brief Finalize mixing
 */
#define FINALIZE32(h) \
    do { \
        h ^= h >> 16; \
        h *= 0x85ebca6b; \
        h ^= h >> 13; \
        h *= 0xc2b2ae35; \
        h ^= h >> 16; \
    } while (0)

/**
 * @brief MurmurHash3 32-bit core implementation
 */
static uint32_t murmur3_32_impl(const void *key, size_t len, uint32_t seed)
{
    const uint8_t *data = (const uint8_t *)key;
    const size_t nblocks = len / 4;

    uint32_t h1 = seed;
    uint32_t c1 = 0xcc9e2d51;  /* Constant 1 */
    uint32_t c2 = 0x1b873593;  /* Constant 2 */
    size_t i;

    /* Process 4-byte blocks */
    for (i = 0; i < nblocks; i++) {
        uint32_t k1 = GETBLOCK32(data, i);

        k1 *= c1;
        k1 = ROTL32(k1, 15);
        k1 *= c2;

        h1 ^= k1;
        h1 = ROTL32(h1, 13);
        h1 = h1 * 5 + 0xe6546b64;
    }

    /* Process remaining bytes */
    const uint8_t *tail = data + nblocks * 4;
    uint32_t k1 = 0;

    switch (len & 3) {
        case 3:
            k1 ^= tail[2] << 16;
            /* fall through */
        case 2:
            k1 ^= tail[1] << 8;
            /* fall through */
        case 1:
            k1 ^= tail[0];
            k1 *= c1;
            k1 = ROTL32(k1, 15);
            k1 *= c2;
            h1 ^= k1;
            break;
    }

    /* Finalize mixing */
    h1 ^= (uint32_t)len;
    FINALIZE32(h1);

    return h1;
}

/* ============================================
 * Interface Implementation
 * ============================================ */

uint32_t murmur3_32(const void *key, size_t len)
{
    return murmur3_32_impl(key, len, 0xc28f5ec5);  /* Default seed */
}

uint32_t murmur3_32_seed(const void *key, size_t len, uint32_t seed)
{
    return murmur3_32_impl(key, len, seed);
}

/* ============================================
 * Integer Hash Implementation
 * ============================================ */

uint32_t dict_number_hash(const void *key, size_t key_size)
{
    uint32_t h = 0;

    switch (key_size) {
        case 1:
            h = *(const uint8_t *)key;
            break;
        case 2:
            h = *(const uint16_t *)key;
            break;
        case 4:
            h = *(const uint32_t *)key;
            break;
        case 8: {
            /* int64 requires more complex mixing to reduce collisions */
            uint64_t v = *(const uint64_t *)key;
            /* Use splitmix32 algorithm */
            uint64_t z = v + 0x9e3779b97f4a7c15ULL;
            z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
            z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
            h = (uint32_t)(z ^ (z >> 31));
            break;
        }
        default:
            /* Unsupported key length, fall back to MurmurHash3 */
            return murmur3_32(key, key_size);
    }

    /* Bloom filter hash mixing */
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;

    return h;
}
