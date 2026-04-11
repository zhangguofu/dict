/**
 * @file dict_hash.h
 * @brief Hash algorithm implementation
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
#ifndef __DICT_HASH_H__
#define __DICT_HASH_H__

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Safe memcpy macro for hash operations (avoids ARM32 unaligned access issues) */
#define DICT_HASH_MEMCPY(dst, src, n)   memcpy((dst), (src), (n))

/**
 * @brief MurmurHash3 32-bit hash function (uses default seed 0xc28f5ec5)
 * @param key Key data
 * @param len Key length
 * @return 32-bit hash value
 */
uint32_t murmur3_32(const void *key, size_t len);

/**
 * @brief MurmurHash3 32-bit hash function (specify seed)
 * @param key Key data
 * @param len Key length
 * @param seed Seed value
 * @return 32-bit hash value
 */
uint32_t murmur3_32_seed(const void *key, size_t len, uint32_t seed);

/**
 * @brief Integer hash function (optimized version, supports 1/2/4/8 bytes)
 * @param key Key data
 * @param key_size Key length (1/2/4/8)
 * @return 32-bit hash value
 */
uint32_t dict_number_hash(const void *key, size_t key_size);

#ifdef __cplusplus
}
#endif

#endif /* __DICT_HASH_H__ */
