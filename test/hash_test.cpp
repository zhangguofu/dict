/**
 * @file hash_test.cpp
 * @brief MurmurHash3 哈希算法单元测试
 */

#include <gtest/gtest.h>
#include <cstring>
#include <cstdint>
#include "dict.h"
#include "dict_hash.h"

/* ============================================
 * 测试验证集（使用默认种子 0xc28f5ec5）
 * ============================================ */

typedef struct {
    const char *key;     /* 输入字符串 */
    size_t len;          /* 长度 */
    uint32_t hash;       /* 期望的哈希值 */
} murmur3_test_vector_t;

static const murmur3_test_vector_t g_test_vectors[] = {
    /* 空字符串 */
    {"",                      0,  0x373AA46D},
    {"a",                     1,  0xE2D9D15B},
    {"ab",                    2,  0xF78B5B88},
    {"abc",                   3,  0x221D171B},
    {"abcd",                  4,  0x2DF7AF0B},
    {"abcde",                 5,  0xC4D42531},
    {"abcdef",                6,  0xF8D8C134},
    {"abcdefg",               7,  0x91749150},
    {"abcdefgh",              8,  0x199AB6AF},
    {"abcdefghi",             9,  0xF4A9C91A},
    {"abcdefghij",           10,  0xB38EB0A4},
    {"abcdefghijk",          11,  0x2DCEEA03},
    {"abcdefghijkl",         12,  0x9B0E7794},
    {"abcdefghijklm",        13,  0x54BFC79E},
    {"abcdefghijklmn",       14,  0x1DB20FA1},
    {"abcdefghijklmno",      15,  0x5531A2B9},
    {"abcdefghijklmnop",     16,  0x47579475},

    /* 常见键名 */
    {"key",                   3,  0x6C3EEC4C},
    {"key1",                  4,  0x179AC378},
    {"key12",                 5,  0x6BCC54CC},
    {"key123",                6,  0x75C34442},
    {"hello",                 5,  0xD202BBF6},
    {"hello world",          11,  0xB7CDB532},
    {"test",                  4,  0xEDC95132},
    {"dict",                  4,  0xD629DC3D},

    /* 相同前缀不同长度 */
    {"aaaa",                  4,  0xFDDE4C02},
    {"aaaaa",                 5,  0x326BDF01},
    {"aaaaaa",                6,  0x6DFC4C28},
    {"aaaab",                 5,  0xE8E80128},

    /* 数字序列 */
    {"0",                     1,  0xD1A9BF32},
    {"1",                     1,  0xF4FBEDAD},
    {"12",                    2,  0x6CCEFCB5},
    {"123",                   3,  0x23031B14},
    {"1234",                  4,  0x7A90B46D},
};

#define TEST_VECTOR_COUNT (sizeof(g_test_vectors) / sizeof(g_test_vectors[0]))

/* ============================================
 * 测试用例：MurmurHash3 标准验证集
 * ============================================ */

class MurmurHashTest : public ::testing::Test {};

TEST_F(MurmurHashTest, VerifyInternalVectors)
{
    /* 使用默认种子测试内部验证集 */
    const murmur3_test_vector_t *vectors = g_test_vectors;
    size_t count = TEST_VECTOR_COUNT;

    ASSERT_GT(count, (size_t)0);

    for (size_t i = 0; i < count; i++) {
        uint32_t hash = murmur3_32(vectors[i].key, vectors[i].len);
        EXPECT_EQ(hash, vectors[i].hash)
            << "Failed at index " << i << ": key=\"" << vectors[i].key << "\""
            << " (len=" << vectors[i].len << ")"
            << " expected=0x" << std::hex << vectors[i].hash
            << " got=0x" << hash << std::dec;
    }
}

TEST_F(MurmurHashTest, EmptyString)
{
    uint32_t h = murmur3_32("", 0);
    EXPECT_EQ(h, 0x373AA46Du);
}

TEST_F(MurmurHashTest, SingleChar)
{
    uint32_t h = murmur3_32("a", 1);
    EXPECT_EQ(h, 0xE2D9D15Bu);
}

TEST_F(MurmurHashTest, PartialLength)
{
    /* 不同长度的同一字符串前缀应有不同哈希 */
    uint32_t h1 = murmur3_32("abc", 1);  /* "a" */
    uint32_t h2 = murmur3_32("abc", 2);  /* "ab" */
    uint32_t h3 = murmur3_32("abc", 3); /* "abc" */

    EXPECT_NE(h1, h2);
    EXPECT_NE(h2, h3);
    EXPECT_NE(h1, h3);
}

TEST_F(MurmurHashTest, DifferentSeeds)
{
    const char *key = "test";

    uint32_t h1 = murmur3_32_seed(key, 4, 0);
    uint32_t h2 = murmur3_32_seed(key, 4, 1);
    uint32_t h3 = murmur3_32_seed(key, 4, 0xFFFFFFFF);

    EXPECT_NE(h1, h2);
    EXPECT_NE(h2, h3);
    EXPECT_NE(h1, h3);
}

TEST_F(MurmurHashTest, CollisionResistance)
{
    /* 验证相似字符串产生不同哈希 */
    const char *keys[] = {
        "key", "key1", "key2", "kye", "yek", "ky1", "1key", "keya"
    };

    for (int i = 0; i < 8; i++) {
        uint32_t h1 = murmur3_32(keys[i], strlen(keys[i]));
        for (int j = i + 1; j < 8; j++) {
            uint32_t h2 = murmur3_32(keys[j], strlen(keys[j]));
            EXPECT_NE(h1, h2) << "Collision between \"" << keys[i]
                              << "\" and \"" << keys[j] << "\"";
        }
    }
}

TEST_F(MurmurHashTest, SameInputSameOutput)
{
    /* 相同输入必须产生相同输出 */
    const char *key = "consistent_test";

    for (int i = 0; i < 100; i++) {
        uint32_t h = murmur3_32(key, strlen(key));
        EXPECT_EQ(h, murmur3_32(key, strlen(key)));
    }
}
