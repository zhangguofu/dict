/**
 * @file dict_hash.h
 * @brief Hash algorithm implementation
 */
#ifndef __DICT_HASH_H__
#define __DICT_HASH_H__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

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
