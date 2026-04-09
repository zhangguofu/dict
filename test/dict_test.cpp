/**
 * @file dict_test.cpp
 * @brief 通用字典模块单元测试
 */

#include <gtest/gtest.h>
#include <cstring>
#include <stdint.h>
#include "dict.h"
#include "dict_hash.h"

/* ============================================
 * 测试数据结构
 * ============================================ */

typedef struct {
    uint16_t id;
    uint16_t type;
} BinaryKey;

/* ============================================
 * 测试夹具类
 * ============================================ */

class DictStringTest : public ::testing::Test {
protected:
    void SetUp() override {
        dict_config_t config = {
            .capacity = 32,
            .key_type = DICT_KEY_STRING,
            .key_size = 0,
            .hash_fn = NULL
        };
        dict_ = dict_create(&config);
        ASSERT_TRUE(dict_ != nullptr);
    }

    void TearDown() override {
        if (dict_) {
            dict_destroy(dict_);
            dict_ = nullptr;
        }
    }

    dict_handle_t dict_;
};

class DictNumberTest : public ::testing::Test {
protected:
    void SetUp() override {
        dict_config_t config = {
            .capacity = 32,
            .key_type = DICT_KEY_NUMBER,
            .key_size = sizeof(int32_t),
            .hash_fn = NULL
        };
        dict_ = dict_create(&config);
        ASSERT_TRUE(dict_ != nullptr);
    }

    void TearDown() override {
        if (dict_) {
            dict_destroy(dict_);
            dict_ = nullptr;
        }
    }

    dict_handle_t dict_;
};

class DictBinaryTest : public ::testing::Test {
protected:
    void SetUp() override {
        dict_config_t config = {
            .capacity = 32,
            .key_type = DICT_KEY_BINARY,
            .key_size = sizeof(BinaryKey),
            .hash_fn = NULL
        };
        dict_ = dict_create(&config);
        ASSERT_TRUE(dict_ != nullptr);
    }

    void TearDown() override {
        if (dict_) {
            dict_destroy(dict_);
            dict_ = nullptr;
        }
    }

    dict_handle_t dict_;
};

/* ============================================
 * 测试用例：配置校验
 * ============================================ */

TEST(DictConfigTest, InvalidKeyType)
{
    /* 无效的 key_type */
    dict_config_t config = {
        .capacity = 32,
        .key_type = (dict_key_type_t)99,  /* 无效类型 */
        .key_size = 0,
        .hash_fn = NULL
    };
    EXPECT_EQ(dict_create(&config), nullptr);
}

TEST(DictConfigTest, NumberWithoutKeySize)
{
    /* NUMBER 类型必须指定 key_size */
    dict_config_t config = {
        .capacity = 32,
        .key_type = DICT_KEY_NUMBER,
        .key_size = 0,  /* 错误：未指定 */
        .hash_fn = NULL
    };
    EXPECT_EQ(dict_create(&config), nullptr);
}

TEST(DictConfigTest, BinaryWithoutKeySize)
{
    /* BINARY 类型必须指定 key_size */
    dict_config_t config = {
        .capacity = 32,
        .key_type = DICT_KEY_BINARY,
        .key_size = 0,  /* 错误：未指定 */
        .hash_fn = NULL
    };
    EXPECT_EQ(dict_create(&config), nullptr);
}

TEST(DictConfigTest, NullConfig)
{
    /* NULL 配置使用默认配置（STRING类型） */
    dict_handle_t dict = dict_create(NULL);
    EXPECT_NE(dict, nullptr);
    dict_destroy(dict);
}

TEST(DictConfigTest, CapacityAlignment)
{
    /* 非2的幂次容量应向上对齐到2的幂次 */
    dict_config_t config = {
        .capacity = 10,
        .key_type = DICT_KEY_STRING,
        .key_size = 0,
        .hash_fn = NULL
    };
    dict_handle_t dict = dict_create(&config);
    EXPECT_NE(dict, nullptr);
    dict_destroy(dict);
}

/* ============================================
 * 测试用例：创建和销毁
 * ============================================ */

TEST(DictCreateTest, DefaultConfig)
{
    dict_handle_t dict = dict_create(NULL);
    EXPECT_NE(dict, nullptr);
    EXPECT_EQ(dict_size(dict), 0u);
    EXPECT_EQ(dict_destroy(dict), DICT_OK);
}

TEST(DictCreateTest, CustomCapacity)
{
    dict_config_t config = {
        .capacity = 64,
        .key_type = DICT_KEY_STRING,
        .key_size = 0,
        .hash_fn = NULL
    };
    dict_handle_t dict = dict_create(&config);
    EXPECT_NE(dict, nullptr);
    EXPECT_EQ(dict_destroy(dict), DICT_OK);
}

TEST(DictCreateTest, DestroyNull)
{
    EXPECT_EQ(dict_destroy(NULL), DICT_EINVALID);
}

/* ============================================
 * 测试用例：STRING 类型 - 基础操作
 * ============================================ */

TEST_F(DictStringTest, SetAndGetInt)
{
    int value = 42;
    EXPECT_EQ(dict_set(dict_, "int_key", &value, sizeof(int)), DICT_OK);
    EXPECT_EQ(dict_size(dict_), 1u);

    int out_value;
    EXPECT_EQ(dict_get(dict_, "int_key", &out_value, sizeof(int)), DICT_OK);
    EXPECT_EQ(out_value, 42);
}

TEST_F(DictStringTest, SetAndGetFloat)
{
    float value = 3.14f;
    EXPECT_EQ(dict_set(dict_, "float_key", &value, sizeof(float)), DICT_OK);

    float out_value;
    EXPECT_EQ(dict_get(dict_, "float_key", &out_value, sizeof(float)), DICT_OK);
    EXPECT_FLOAT_EQ(out_value, 3.14f);
}

TEST_F(DictStringTest, SetAndGetString)
{
    const char *str = "hello world";
    EXPECT_EQ(dict_set(dict_, "str_key", str, strlen(str) + 1), DICT_OK);
    EXPECT_EQ(dict_size(dict_), 1u);

    char buf[64];
    EXPECT_EQ(dict_get(dict_, "str_key", buf, sizeof(buf)), DICT_OK);
    EXPECT_STREQ(buf, str);
}

TEST_F(DictStringTest, MultipleTypes)
{
    int int_val = 100;
    float float_val = 3.14f;
    const char *str_val = "test";

    EXPECT_EQ(dict_set(dict_, "int", &int_val, sizeof(int)), DICT_OK);
    EXPECT_EQ(dict_set(dict_, "float", &float_val, sizeof(float)), DICT_OK);
    EXPECT_EQ(dict_set(dict_, "str", str_val, strlen(str_val) + 1), DICT_OK);
    EXPECT_EQ(dict_size(dict_), 3u);
}

/* ============================================
 * 测试用例：STRING 类型 - 结构体存储
 * ============================================ */

typedef struct {
    char name[32];
    int id;
    float score;
} UserInfo;

TEST_F(DictStringTest, SetAndGetStruct)
{
    UserInfo user1 = {"ZhangSan", 1001, 95.5f};
    EXPECT_EQ(dict_set(dict_, "user1", &user1, sizeof(UserInfo)), DICT_OK);

    UserInfo out_user;
    EXPECT_EQ(dict_get(dict_, "user1", &out_user, sizeof(UserInfo)), DICT_OK);
    EXPECT_STREQ(out_user.name, "ZhangSan");
    EXPECT_EQ(out_user.id, 1001);
    EXPECT_FLOAT_EQ(out_user.score, 95.5f);
}

TEST_F(DictStringTest, OverwriteStruct)
{
    UserInfo user1 = {"ZhangSan", 1001, 95.5f};
    dict_set(dict_, "user", &user1, sizeof(UserInfo));

    UserInfo user2 = {"LiSi", 1002, 88.0f};
    EXPECT_EQ(dict_set(dict_, "user", &user2, sizeof(UserInfo)), DICT_OK);

    UserInfo out_user;
    EXPECT_EQ(dict_get(dict_, "user", &out_user, sizeof(UserInfo)), DICT_OK);
    EXPECT_STREQ(out_user.name, "LiSi");
    EXPECT_EQ(out_user.id, 1002);
    EXPECT_EQ(dict_size(dict_), 1u);  /* 大小不变 */
}

/* ============================================
 * 测试用例：STRING 类型 - 查询操作
 * ============================================ */

TEST_F(DictStringTest, GetNotFound)
{
    int value;
    EXPECT_EQ(dict_get(dict_, "nonexist", &value, sizeof(value)), DICT_ENOTFOUND);
}

TEST_F(DictStringTest, GetAfterDelete)
{
    int val = 100;
    EXPECT_EQ(dict_set(dict_, "key", &val, sizeof(val)), DICT_OK);
    EXPECT_EQ(dict_delete(dict_, "key"), DICT_OK);

    int value;
    EXPECT_EQ(dict_get(dict_, "key", &value, sizeof(value)), DICT_ENOTFOUND);
}

/* ============================================
 * 测试用例：STRING 类型 - 获取值大小
 * ============================================ */

TEST_F(DictStringTest, GetSize)
{
    int int_val = 100;
    EXPECT_EQ(dict_set(dict_, "int", &int_val, sizeof(int)), DICT_OK);

    size_t size;
    EXPECT_EQ(dict_get_size(dict_, "int", &size), DICT_OK);
    EXPECT_EQ(size, sizeof(int));
}

TEST_F(DictStringTest, GetSizeNotFound)
{
    size_t size;
    EXPECT_EQ(dict_get_size(dict_, "nonexist", &size), DICT_ENOTFOUND);
}

TEST_F(DictStringTest, GetSizeNullParam)
{
    EXPECT_EQ(dict_get_size(dict_, "key", NULL), DICT_EINVALID);
}

/* ============================================
 * 测试用例：STRING 类型 - 删除操作
 * ============================================ */

TEST_F(DictStringTest, DeleteExisting)
{
    int val = 100;
    EXPECT_EQ(dict_set(dict_, "key1", &val, sizeof(val)), DICT_OK);
    EXPECT_EQ(dict_set(dict_, "key2", &val, sizeof(val)), DICT_OK);
    EXPECT_EQ(dict_size(dict_), 2u);

    EXPECT_EQ(dict_delete(dict_, "key1"), DICT_OK);
    EXPECT_EQ(dict_size(dict_), 1u);
}

TEST_F(DictStringTest, DeleteNotFound)
{
    EXPECT_EQ(dict_delete(dict_, "nonexist"), DICT_ENOTFOUND);
}

TEST_F(DictStringTest, DeleteNullKey)
{
    EXPECT_EQ(dict_delete(dict_, NULL), DICT_EINVALID);
}

/* ============================================
 * 测试用例：STRING 类型 - 覆盖已有键值
 * ============================================ */

TEST_F(DictStringTest, OverwriteWithSameSize)
{
    int val1 = 100;
    int val2 = 200;
    EXPECT_EQ(dict_set(dict_, "key", &val1, sizeof(int)), DICT_OK);
    EXPECT_EQ(dict_set(dict_, "key", &val2, sizeof(int)), DICT_OK);

    int out_val;
    EXPECT_EQ(dict_get(dict_, "key", &out_val, sizeof(int)), DICT_OK);
    EXPECT_EQ(out_val, 200);
}

TEST_F(DictStringTest, OverwriteWithDifferentSize)
{
    int int_val = 100;
    long long ll_val = 1234567890LL;

    EXPECT_EQ(dict_set(dict_, "key", &int_val, sizeof(int)), DICT_OK);
    EXPECT_EQ(dict_set(dict_, "key", &ll_val, sizeof(long long)), DICT_OK);

    size_t size;
    EXPECT_EQ(dict_get_size(dict_, "key", &size), DICT_OK);
    EXPECT_EQ(size, sizeof(long long));

    long long out_val;
    EXPECT_EQ(dict_get(dict_, "key", &out_val, sizeof(long long)), DICT_OK);
    EXPECT_EQ(out_val, 1234567890LL);
}

TEST_F(DictStringTest, MultipleOverwrites)
{
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(dict_set(dict_, "key", &i, sizeof(int)), DICT_OK);
    }

    int out_val;
    EXPECT_EQ(dict_get(dict_, "key", &out_val, sizeof(int)), DICT_OK);
    EXPECT_EQ(out_val, 9);
    EXPECT_EQ(dict_size(dict_), 1u);
}

/* ============================================
 * 测试用例：STRING 类型 - 清空操作
 * ============================================ */

TEST_F(DictStringTest, ClearNull)
{
    EXPECT_EQ(dict_clear(NULL), DICT_EINVALID);
}

TEST_F(DictStringTest, ClearEmpty)
{
    EXPECT_EQ(dict_clear(dict_), DICT_OK);
    EXPECT_EQ(dict_size(dict_), 0u);
}

TEST_F(DictStringTest, ClearWithData)
{
    int val = 100;
    for (int i = 0; i < 10; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);
        EXPECT_EQ(dict_set(dict_, key, &val, sizeof(val)), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict_), 10u);

    EXPECT_EQ(dict_clear(dict_), DICT_OK);
    EXPECT_EQ(dict_size(dict_), 0u);

    /* 清空后可继续使用 */
    EXPECT_EQ(dict_set(dict_, "new_key", &val, sizeof(val)), DICT_OK);
    EXPECT_EQ(dict_size(dict_), 1u);
}

/* ============================================
 * 测试用例：STRING 类型 - 空值处理
 * ============================================ */

TEST_F(DictStringTest, NullValue)
{
    EXPECT_EQ(dict_set(dict_, "empty", NULL, 0), DICT_OK);
    EXPECT_EQ(dict_size(dict_), 1u);

    size_t size;
    EXPECT_EQ(dict_get_size(dict_, "empty", &size), DICT_OK);
    EXPECT_EQ(size, 0u);
}

TEST_F(DictStringTest, OverwriteNullWithValue)
{
    EXPECT_EQ(dict_set(dict_, "key", NULL, 0), DICT_OK);

    int val = 100;
    EXPECT_EQ(dict_set(dict_, "key", &val, sizeof(int)), DICT_OK);

    int out_val;
    EXPECT_EQ(dict_get(dict_, "key", &out_val, sizeof(int)), DICT_OK);
    EXPECT_EQ(out_val, 100);
}

/* ============================================
 * 测试用例：STRING 类型 - 边界条件
 * ============================================ */

TEST_F(DictStringTest, NullParams)
{
    int value = 100;
    EXPECT_EQ(dict_set(NULL, "key", &value, sizeof(value)), DICT_EINVALID);
    EXPECT_EQ(dict_set(dict_, NULL, &value, sizeof(value)), DICT_EINVALID);
    EXPECT_EQ(dict_get(NULL, "key", &value, sizeof(value)), DICT_EINVALID);
    EXPECT_EQ(dict_get(dict_, NULL, &value, sizeof(value)), DICT_EINVALID);
    EXPECT_EQ(dict_delete(NULL, "key"), DICT_EINVALID);
    EXPECT_EQ(dict_delete(dict_, NULL), DICT_EINVALID);
}

TEST_F(DictStringTest, BufferTooSmall)
{
    int value = 12345;
    EXPECT_EQ(dict_set(dict_, "int", &value, sizeof(int)), DICT_OK);

    char buf[4];
    EXPECT_EQ(dict_get(dict_, "int", buf, 0), DICT_ETOOSMALL);
    EXPECT_EQ(dict_get(dict_, "int", buf, 1), DICT_ETOOSMALL);
    EXPECT_EQ(dict_get(dict_, "int", buf, sizeof(int)), DICT_OK);
}

TEST_F(DictStringTest, EmptyKey)
{
    int value = 100;
    EXPECT_EQ(dict_set(dict_, "", &value, sizeof(value)), DICT_OK);

    int out_val;
    EXPECT_EQ(dict_get(dict_, "", &out_val, sizeof(int)), DICT_OK);
    EXPECT_EQ(out_val, 100);
}

TEST_F(DictStringTest, SpecialCharsInKey)
{
    int value = 100;
    EXPECT_EQ(dict_set(dict_, "key-with-dash_and_underscore", &value, sizeof(value)), DICT_OK);

    int out_val;
    EXPECT_EQ(dict_get(dict_, "key-with-dash_and_underscore", &out_val, sizeof(int)), DICT_OK);
    EXPECT_EQ(out_val, 100);
}

TEST_F(DictStringTest, LongKey)
{
    char long_key[256];
    memset(long_key, 'a', sizeof(long_key) - 1);
    long_key[sizeof(long_key) - 1] = '\0';

    int value = 100;
    EXPECT_EQ(dict_set(dict_, long_key, &value, sizeof(value)), DICT_OK);

    int out_val;
    EXPECT_EQ(dict_get(dict_, long_key, &out_val, sizeof(int)), DICT_OK);
    EXPECT_EQ(out_val, 100);
}

/* ============================================
 * 测试用例：NUMBER 类型 - int32_t 键
 * ============================================ */

TEST_F(DictNumberTest, SetAndGetInt32)
{
    int32_t key = 100;
    int value = 42;
    EXPECT_EQ(dict_set(dict_, &key, &value, sizeof(int)), DICT_OK);
    EXPECT_EQ(dict_size(dict_), 1u);

    int out_value;
    EXPECT_EQ(dict_get(dict_, &key, &out_value, sizeof(int)), DICT_OK);
    EXPECT_EQ(out_value, 42);
}

TEST_F(DictNumberTest, MultipleKeys)
{
    int32_t keys[] = {1, 2, 3, 4, 5};
    int values[] = {10, 20, 30, 40, 50};

    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(dict_set(dict_, &keys[i], &values[i], sizeof(int)), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict_), 5u);

    /* 验证所有键值对 */
    for (int i = 0; i < 5; i++) {
        int out_val;
        EXPECT_EQ(dict_get(dict_, &keys[i], &out_val, sizeof(int)), DICT_OK);
        EXPECT_EQ(out_val, values[i]);
    }
}

TEST_F(DictNumberTest, OverwriteKey)
{
    int32_t key = 100;
    int val1 = 10;
    int val2 = 20;

    EXPECT_EQ(dict_set(dict_, &key, &val1, sizeof(int)), DICT_OK);
    EXPECT_EQ(dict_set(dict_, &key, &val2, sizeof(int)), DICT_OK);
    EXPECT_EQ(dict_size(dict_), 1u);

    int out_val;
    EXPECT_EQ(dict_get(dict_, &key, &out_val, sizeof(int)), DICT_OK);
    EXPECT_EQ(out_val, 20);
}

TEST_F(DictNumberTest, DeleteKey)
{
    int32_t key1 = 1, key2 = 2;
    int val = 100;

    EXPECT_EQ(dict_set(dict_, &key1, &val, sizeof(val)), DICT_OK);
    EXPECT_EQ(dict_set(dict_, &key2, &val, sizeof(val)), DICT_OK);
    EXPECT_EQ(dict_delete(dict_, &key1), DICT_OK);
    EXPECT_EQ(dict_size(dict_), 1u);

    /* key1 已删除 */
    EXPECT_EQ(dict_get(dict_, &key1, &val, sizeof(val)), DICT_ENOTFOUND);
    EXPECT_EQ(dict_delete(dict_, &key1), DICT_ENOTFOUND);

    /* key2 仍在 */
    EXPECT_EQ(dict_get(dict_, &key2, &val, sizeof(val)), DICT_OK);
}

TEST_F(DictNumberTest, NotFound)
{
    int32_t key = 999;
    int value;
    EXPECT_EQ(dict_get(dict_, &key, &value, sizeof(value)), DICT_ENOTFOUND);
    EXPECT_EQ(dict_delete(dict_, &key), DICT_ENOTFOUND);
}

/* ============================================
 * 测试用例：NUMBER 类型 - int64_t 键
 * ============================================ */

TEST(DictNumberInt64Test, Int64Key)
{
    dict_config_t config = {
        .capacity = 32,
        .key_type = DICT_KEY_NUMBER,
        .key_size = sizeof(int64_t),
        .hash_fn = NULL
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);

    int64_t key = 12345678901234LL;
    int value = 42;
    EXPECT_EQ(dict_set(dict, &key, &value, sizeof(value)), DICT_OK);

    int out_val;
    EXPECT_EQ(dict_get(dict, &key, &out_val, sizeof(out_val)), DICT_OK);
    EXPECT_EQ(out_val, 42);

    dict_destroy(dict);
}

/* ============================================
 * 测试用例：NUMBER 类型 - short 键
 * ============================================ */

TEST(DictNumberShortTest, ShortKey)
{
    dict_config_t config = {
        .capacity = 32,
        .key_type = DICT_KEY_NUMBER,
        .key_size = sizeof(short),
        .hash_fn = NULL
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);

    short key = 100;
    int value = 42;
    EXPECT_EQ(dict_set(dict, &key, &value, sizeof(value)), DICT_OK);

    int out_val;
    EXPECT_EQ(dict_get(dict, &key, &out_val, sizeof(out_val)), DICT_OK);
    EXPECT_EQ(out_val, 42);

    dict_destroy(dict);
}

TEST(DictNumberShortTest, MultipleShortKeys)
{
    dict_config_t config = {
        .capacity = 64,
        .key_type = DICT_KEY_NUMBER,
        .key_size = sizeof(short),
        .hash_fn = NULL
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);

    /* 插入多个 short 键 */
    for (short i = 0; i < 50; i++) {
        int value = i * 10;
        EXPECT_EQ(dict_set(dict, &i, &value, sizeof(value)), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict), 50u);

    /* 验证所有数据 */
    for (short i = 0; i < 50; i++) {
        int out_val;
        EXPECT_EQ(dict_get(dict, &i, &out_val, sizeof(out_val)), DICT_OK);
        EXPECT_EQ(out_val, i * 10);
    }

    /* 删除部分键 */
    for (short i = 0; i < 25; i++) {
        EXPECT_EQ(dict_delete(dict, &i), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict), 25u);

    /* 验证剩余数据 */
    for (short i = 25; i < 50; i++) {
        int out_val;
        EXPECT_EQ(dict_get(dict, &i, &out_val, sizeof(out_val)), DICT_OK);
        EXPECT_EQ(out_val, i * 10);
    }

    dict_destroy(dict);
}

TEST(DictNumberShortTest, BoundaryValues)
{
    dict_config_t config = {
        .capacity = 32,
        .key_type = DICT_KEY_NUMBER,
        .key_size = sizeof(short),
        .hash_fn = NULL
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);

    /* 测试边界值 */
    short keys[] = {0, -1, 1, SHRT_MAX, SHRT_MIN};
    int values[] = {0, -100, 100, 32767, -32768};

    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        EXPECT_EQ(dict_set(dict, &keys[i], &values[i], sizeof(values[i])), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict), 5u);

    /* 验证边界值 */
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        int out_val;
        EXPECT_EQ(dict_get(dict, &keys[i], &out_val, sizeof(out_val)), DICT_OK);
        EXPECT_EQ(out_val, values[i]);
    }

    dict_destroy(dict);
}

/* ============================================
 * 测试用例：NUMBER 类型 - char 键
 * ============================================ */

TEST(DictNumberCharTest, CharKey)
{
    dict_config_t config = {
        .capacity = 32,
        .key_type = DICT_KEY_NUMBER,
        .key_size = sizeof(char),
        .hash_fn = NULL
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);

    char key = 'A';
    int value = 42;
    EXPECT_EQ(dict_set(dict, &key, &value, sizeof(value)), DICT_OK);

    int out_val;
    EXPECT_EQ(dict_get(dict, &key, &out_val, sizeof(out_val)), DICT_OK);
    EXPECT_EQ(out_val, 42);

    dict_destroy(dict);
}

TEST(DictNumberCharTest, MultipleCharKeys)
{
    dict_config_t config = {
        .capacity = 128,
        .key_type = DICT_KEY_NUMBER,
        .key_size = sizeof(char),
        .hash_fn = NULL
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);

    /* 插入所有可能的 char 值（-128 到 127） */
    for (int i = -128; i < 128; i++) {
        char key = (char)i;
        int value = i * 100;
        EXPECT_EQ(dict_set(dict, &key, &value, sizeof(value)), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict), 256u);

    /* 验证所有数据 */
    for (int i = -128; i < 128; i++) {
        char key = (char)i;
        int out_val;
        EXPECT_EQ(dict_get(dict, &key, &out_val, sizeof(out_val)), DICT_OK);
        EXPECT_EQ(out_val, i * 100);
    }

    /* 验证存在性 */
    for (int i = -128; i < 128; i++) {
        char key = (char)i;
        EXPECT_EQ(dict_exists(dict, &key), 1);
    }

    dict_destroy(dict);
}

TEST(DictNumberCharTest, UpdateAndDelete)
{
    dict_config_t config = {
        .capacity = 32,
        .key_type = DICT_KEY_NUMBER,
        .key_size = sizeof(char),
        .hash_fn = NULL
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);

    char keys[] = {'a', 'b', 'c', 'd', 'e'};

    /* 插入 */
    for (size_t i = 0; i < sizeof(keys); i++) {
        int value = i + 1;
        EXPECT_EQ(dict_set(dict, &keys[i], &value, sizeof(value)), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict), 5u);

    /* 更新 */
    for (size_t i = 0; i < sizeof(keys); i++) {
        int new_value = (i + 1) * 100;
        EXPECT_EQ(dict_set(dict, &keys[i], &new_value, sizeof(new_value)), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict), 5u);  // size 不变

    /* 验证更新后的值 */
    for (size_t i = 0; i < sizeof(keys); i++) {
        int out_val;
        EXPECT_EQ(dict_get(dict, &keys[i], &out_val, sizeof(out_val)), DICT_OK);
        EXPECT_EQ(out_val, (int)((i + 1) * 100));
    }

    /* 删除部分 */
    EXPECT_EQ(dict_delete(dict, &keys[0]), DICT_OK);
    EXPECT_EQ(dict_delete(dict, &keys[2]), DICT_OK);
    EXPECT_EQ(dict_delete(dict, &keys[4]), DICT_OK);
    EXPECT_EQ(dict_size(dict), 2u);

    /* 验证删除后 */
    EXPECT_EQ(dict_exists(dict, &keys[0]), 0);
    EXPECT_EQ(dict_exists(dict, &keys[2]), 0);
    EXPECT_EQ(dict_exists(dict, &keys[4]), 0);
    EXPECT_EQ(dict_exists(dict, &keys[1]), 1);
    EXPECT_EQ(dict_exists(dict, &keys[3]), 1);

    dict_destroy(dict);
}

/* ============================================
 * 测试用例：BINARY 类型 - 结构体键
 * ============================================ */

TEST_F(DictBinaryTest, StructKey)
{
    BinaryKey key1 = {1, 10};
    BinaryKey key2 = {2, 20};
    int val1 = 100, val2 = 200;

    EXPECT_EQ(dict_set(dict_, &key1, &val1, sizeof(val1)), DICT_OK);
    EXPECT_EQ(dict_set(dict_, &key2, &val2, sizeof(val2)), DICT_OK);
    EXPECT_EQ(dict_size(dict_), 2u);

    /* 验证 */
    int out_val;
    EXPECT_EQ(dict_get(dict_, &key1, &out_val, sizeof(out_val)), DICT_OK);
    EXPECT_EQ(out_val, 100);
    EXPECT_EQ(dict_get(dict_, &key2, &out_val, sizeof(out_val)), DICT_OK);
    EXPECT_EQ(out_val, 200);
}

TEST_F(DictBinaryTest, DeleteStructKey)
{
    BinaryKey key1 = {1, 10};
    BinaryKey key2 = {2, 20};
    int val1 = 100, val2 = 200;

    EXPECT_EQ(dict_set(dict_, &key1, &val1, sizeof(val1)), DICT_OK);
    EXPECT_EQ(dict_set(dict_, &key2, &val2, sizeof(val2)), DICT_OK);
    EXPECT_EQ(dict_delete(dict_, &key1), DICT_OK);
    EXPECT_EQ(dict_size(dict_), 1u);

    int out_val;
    EXPECT_EQ(dict_get(dict_, &key1, &out_val, sizeof(out_val)), DICT_ENOTFOUND);
    EXPECT_EQ(dict_get(dict_, &key2, &out_val, sizeof(out_val)), DICT_OK);
}

TEST_F(DictBinaryTest, StructKeyNoCollision)
{
    /* 不同的结构体值应该有不同的哈希 */
    BinaryKey key1 = {1, 10};
    BinaryKey key2 = {10, 1};  /* 值相同但顺序不同，应该被视为不同的键 */
    int val1 = 100, val2 = 200;

    EXPECT_EQ(dict_set(dict_, &key1, &val1, sizeof(val1)), DICT_OK);
    EXPECT_EQ(dict_set(dict_, &key2, &val2, sizeof(val2)), DICT_OK);
    EXPECT_EQ(dict_size(dict_), 2u);

    int out_val;
    EXPECT_EQ(dict_get(dict_, &key1, &out_val, sizeof(out_val)), DICT_OK);
    EXPECT_EQ(out_val, 100);
    EXPECT_EQ(dict_get(dict_, &key2, &out_val, sizeof(out_val)), DICT_OK);
    EXPECT_EQ(out_val, 200);
}

/* ============================================
 * 测试用例：扩容测试
 * ============================================ */

TEST(DictResizeTest, AutoResize)
{
    dict_config_t config = {
        .capacity = 4,
        .key_type = DICT_KEY_STRING,
        .key_size = 0,
        .hash_fn = NULL
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);

    /* 插入超过初始容量的数据，触发扩容 */
    for (int i = 0; i < 20; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);
        EXPECT_EQ(dict_set(dict, key, &i, sizeof(i)), DICT_OK);
    }

    EXPECT_EQ(dict_size(dict), 20u);

    /* 验证所有数据都正确 */
    for (int i = 0; i < 20; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);
        int out_val;
        EXPECT_EQ(dict_get(dict, key, &out_val, sizeof(int)), DICT_OK);
        EXPECT_EQ(out_val, i);
    }

    dict_destroy(dict);
}

TEST(DictResizeTest, NumberTypeResize)
{
    dict_config_t config = {
        .capacity = 4,
        .key_type = DICT_KEY_NUMBER,
        .key_size = sizeof(int32_t),
        .hash_fn = NULL
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);

    /* 插入大量数据触发扩容 */
    for (int32_t i = 0; i < 50; i++) {
        EXPECT_EQ(dict_set(dict, &i, &i, sizeof(i)), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict), 50u);

    /* 验证所有数据 */
    for (int32_t i = 0; i < 50; i++) {
        int out_val;
        EXPECT_EQ(dict_get(dict, &i, &out_val, sizeof(out_val)), DICT_OK);
        EXPECT_EQ(out_val, i);
    }

    dict_destroy(dict);
}

/* ============================================
 * 测试用例：哈希冲突测试（通过自定义哈希函数）
 * ============================================ */

/* Mock 哈希函数：强制让 "key1" 和 "key2" 返回相同哈希值 */
static uint32_t mock_hash_collision(const void *key, size_t len)
{
    (void)len;
    if (DICT_STRCMP((const char *)key, "key1") == 0) {
        return 0x12345678;
    }
    if (DICT_STRCMP((const char *)key, "key2") == 0) {
        return 0x12345678;
    }
    return murmur3_32(key, len);
}

TEST(DictCustomHashTest, HashCollisionViaCustomHash)
{
    dict_config_t config = {
        .capacity = 32,
        .key_type = DICT_KEY_STRING,
        .key_size = 0,
        .hash_fn = mock_hash_collision
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);

    /* 插入两个键，它们会哈希到同一个桶 */
    int val1 = 100, val2 = 200;
    EXPECT_EQ(dict_set(dict, "key1", &val1, sizeof(int)), DICT_OK);
    EXPECT_EQ(dict_set(dict, "key2", &val2, sizeof(int)), DICT_OK);

    /* 验证两个键都能正确读取 */
    int out1, out2;
    EXPECT_EQ(dict_get(dict, "key1", &out1, sizeof(int)), DICT_OK);
    EXPECT_EQ(out1, 100);
    EXPECT_EQ(dict_get(dict, "key2", &out2, sizeof(int)), DICT_OK);
    EXPECT_EQ(out2, 200);

    /* 覆盖其中一个键 */
    int new_val = 999;
    EXPECT_EQ(dict_set(dict, "key1", &new_val, sizeof(int)), DICT_OK);
    EXPECT_EQ(dict_size(dict), 2u);

    /* 验证覆盖生效，另一个不受影响 */
    EXPECT_EQ(dict_get(dict, "key1", &out1, sizeof(int)), DICT_OK);
    EXPECT_EQ(out1, 999);
    EXPECT_EQ(dict_get(dict, "key2", &out2, sizeof(int)), DICT_OK);
    EXPECT_EQ(out2, 200);

    /* 删除一个，验证另一个仍然存在 */
    EXPECT_EQ(dict_delete(dict, "key1"), DICT_OK);
    EXPECT_EQ(dict_size(dict), 1u);
    EXPECT_EQ(dict_get(dict, "key1", &out1, sizeof(int)), DICT_ENOTFOUND);
    EXPECT_EQ(dict_get(dict, "key2", &out2, sizeof(int)), DICT_OK);
    EXPECT_EQ(out2, 200);

    dict_destroy(dict);
}

/* Mock 哈希函数：所有键都返回相同哈希值（最极端的碰撞） */
static uint32_t mock_hash_all_same(const void *key, size_t len)
{
    (void)key;
    (void)len;
    return 0x87654321;
}

TEST(DictCustomHashTest, ExtremeHashCollision)
{
    dict_config_t config = {
        .capacity = 8,
        .key_type = DICT_KEY_STRING,
        .key_size = 0,
        .hash_fn = mock_hash_all_same
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);

    /* 插入多个键，全部哈希到同一桶 */
    const char *keys[] = {"aaa", "bbb", "ccc", "ddd", "eee"};
    int values[] = {1, 2, 3, 4, 5};
    const int count = 5;

    for (int i = 0; i < count; i++) {
        EXPECT_EQ(dict_set(dict, keys[i], &values[i], sizeof(int)), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict), (size_t)count);

    /* 验证所有键值对 */
    for (int i = 0; i < count; i++) {
        int out_val;
        EXPECT_EQ(dict_get(dict, keys[i], &out_val, sizeof(int)), DICT_OK);
        EXPECT_EQ(out_val, values[i]);
    }

    /* 测试删除和重新插入 */
    EXPECT_EQ(dict_delete(dict, "bbb"), DICT_OK);
    EXPECT_EQ(dict_size(dict), (size_t)(count - 1));

    int new_val = 999;
    EXPECT_EQ(dict_set(dict, "bbb", &new_val, sizeof(int)), DICT_OK);
    EXPECT_EQ(dict_size(dict), (size_t)count);

    /* 验证 */
    int out_val;
    EXPECT_EQ(dict_get(dict, "bbb", &out_val, sizeof(int)), DICT_OK);
    EXPECT_EQ(out_val, 999);

    dict_destroy(dict);
}

/* ============================================
 * 测试用例：哈希分布
 * ============================================ */

TEST(DictHashDistributionTest, ManyEntries)
{
    dict_handle_t dict = dict_create(NULL);
    ASSERT_NE(dict, nullptr);

    const int count = 100;
    for (int i = 0; i < count; i++) {
        char key[32];
        snprintf(key, sizeof(key), "key%d", i);
        EXPECT_EQ(dict_set(dict, key, &i, sizeof(i)), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict), (size_t)count);

    /* 验证所有数据 */
    for (int i = 0; i < count; i++) {
        char key[32];
        snprintf(key, sizeof(key), "key%d", i);
        int out_val;
        EXPECT_EQ(dict_get(dict, key, &out_val, sizeof(int)), DICT_OK);
        EXPECT_EQ(out_val, i);
    }

    /* 删除一半数据 */
    for (int i = 0; i < count; i += 2) {
        char key[32];
        snprintf(key, sizeof(key), "key%d", i);
        EXPECT_EQ(dict_delete(dict, key), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict), (size_t)(count / 2));

    /* 验证剩余数据 */
    for (int i = 1; i < count; i += 2) {
        char key[32];
        snprintf(key, sizeof(key), "key%d", i);
        int out_val;
        EXPECT_EQ(dict_get(dict, key, &out_val, sizeof(int)), DICT_OK);
        EXPECT_EQ(out_val, i);
    }

    dict_destroy(dict);
}

TEST(DictHashDistributionTest, NumberKeysDistribution)
{
    dict_config_t config = {
        .capacity = 16,
        .key_type = DICT_KEY_NUMBER,
        .key_size = sizeof(int32_t),
        .hash_fn = NULL
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);

    /* 插入100个整数键 */
    for (int32_t i = 0; i < 100; i++) {
        EXPECT_EQ(dict_set(dict, &i, &i, sizeof(i)), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict), 100u);

    /* 验证所有数据 */
    for (int32_t i = 0; i < 100; i++) {
        int out_val;
        EXPECT_EQ(dict_get(dict, &i, &out_val, sizeof(out_val)), DICT_OK);
        EXPECT_EQ(out_val, i);
    }

    dict_destroy(dict);
}

/* ============================================
 * 测试用例：整数哈希函数测试
 * ============================================ */

TEST(DictNumberHashTest, DifferentKeySizes)
{
    /* 测试不同长度的键哈希 */
    uint8_t k1 = 42;
    uint16_t k2 = 12345;
    uint32_t k3 = 123456789;
    uint64_t k4 = 123456789012345LL;

    uint32_t h1 = dict_number_hash(&k1, sizeof(k1));
    uint32_t h2 = dict_number_hash(&k2, sizeof(k2));
    uint32_t h3 = dict_number_hash(&k3, sizeof(k3));
    uint32_t h4 = dict_number_hash(&k4, sizeof(k4));

    /* 相同输入应该产生相同哈希 */
    EXPECT_EQ(h1, dict_number_hash(&k1, sizeof(k1)));
    EXPECT_EQ(h2, dict_number_hash(&k2, sizeof(k2)));

    /* 不同输入大概率产生不同哈希 */
    EXPECT_NE(h1, h2);
    EXPECT_NE(h1, h3);
    EXPECT_NE(h1, h4);
}

TEST(DictNumberHashTest, HashConsistency)
{
    /* 相同输入应产生相同输出 */
    uint32_t val = 12345;
    EXPECT_EQ(dict_number_hash(&val, sizeof(val)), dict_number_hash(&val, sizeof(val)));

    /* 不同输入大概率产生不同哈希 */
    uint32_t val1 = 12345;
    uint32_t val2 = 54321;
    EXPECT_NE(dict_number_hash(&val1, sizeof(val1)), dict_number_hash(&val2, sizeof(val2)));
}

/* ============================================
 * 测试用例：边界值测试
 * ============================================ */

TEST(DictBoundaryTest, ZeroCapacity)
{
    dict_config_t config = {
        .capacity = 0,  // 应该使用默认值
        .key_type = DICT_KEY_STRING
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);
    
    // 存储值（带 '\0' 结尾）
    const char *value = "value";
    dict_set(dict, "key", value, strlen(value) + 1);
    EXPECT_EQ(dict_size(dict), 1u);
    
    char buf[16];
    EXPECT_EQ(dict_get(dict, "key", buf, sizeof(buf)), DICT_OK);
    EXPECT_STREQ(buf, "value");
    
    dict_destroy(dict);
}

TEST(DictBoundaryTest, MaxKeyValues)
{
    /* 测试各种特殊值 */
    dict_config_t config = {
        .capacity = 32,
        .key_type = DICT_KEY_STRING
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);
    
    // 空字符串键
    EXPECT_EQ(dict_set(dict, "", "empty_key", 10), DICT_OK);
    EXPECT_EQ(dict_size(dict), 1u);
    
    // 空值
    EXPECT_EQ(dict_set(dict, "null_val", "", 0), DICT_OK);
    EXPECT_EQ(dict_size(dict), 2u);
    
    // 非常长的键
    char long_key[256];
    memset(long_key, 'k', sizeof(long_key) - 1);
    long_key[sizeof(long_key) - 1] = '\0';
    EXPECT_EQ(dict_set(dict, long_key, "long_key", 8), DICT_OK);
    EXPECT_EQ(dict_size(dict), 3u);
    
    // 非常长的值
    char long_val[1024];
    memset(long_val, 'v', sizeof(long_val) - 1);
    long_val[sizeof(long_val) - 1] = '\0';
    EXPECT_EQ(dict_set(dict, "long_val", long_val, sizeof(long_val)), DICT_OK);
    EXPECT_EQ(dict_size(dict), 4u);
    
    dict_destroy(dict);
}

TEST(DictBoundaryTest, IntegerEdgeValues)
{
    dict_config_t config = {
        .capacity = 32,
        .key_type = DICT_KEY_NUMBER,
        .key_size = sizeof(int32_t)
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);
    
    int32_t values[] = {
        0,                           // 零
        1,                           // 最小正整数
        -1,                          // 最大负整数
        INT32_MAX,                   // 最大值
        INT32_MIN,                   // 最小值
    };
    
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(dict_set(dict, &values[i], &i, sizeof(i)), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict), 5u);
    
    // 验证所有值
    int out_val;
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(dict_get(dict, &values[i], &out_val, sizeof(out_val)), DICT_OK);
        EXPECT_EQ(out_val, i);
    }
    
    dict_destroy(dict);
}

TEST(DictBoundaryTest, UnsignedEdgeValues)
{
    dict_config_t config = {
        .capacity = 32,
        .key_type = DICT_KEY_NUMBER,
        .key_size = sizeof(uint64_t)
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);
    
    uint64_t keys[] = {
        0,
        1,
        UINT64_MAX,
        0x8000000000000000ULL,  // 最高位设置
        0x0000000100000000ULL,  // 高低位不同
    };
    
    for (int i = 0; i < 5; i++) {
        int val = i * 100;
        EXPECT_EQ(dict_set(dict, &keys[i], &val, sizeof(val)), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict), 5u);
    
    // 验证
    for (int i = 0; i < 5; i++) {
        int out_val;
        EXPECT_EQ(dict_get(dict, &keys[i], &out_val, sizeof(out_val)), DICT_OK);
        EXPECT_EQ(out_val, i * 100);
    }
    
    dict_destroy(dict);
}

TEST(DictBoundaryTest, BufferExactlySize)
{
    dict_config_t config = {
        .capacity = 32,
        .key_type = DICT_KEY_STRING
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);
    
    const char *value = "12345";
    EXPECT_EQ(dict_set(dict, "key", value, 6), DICT_OK);
    
    // 缓冲区恰好大小
    char buf[6];
    EXPECT_EQ(dict_get(dict, "key", buf, sizeof(buf)), DICT_OK);
    EXPECT_STREQ(buf, value);
    
    // 缓冲区太小（刚好少1字节）
    char buf_small[5];
    EXPECT_EQ(dict_get(dict, "key", buf_small, sizeof(buf_small)), DICT_ETOOSMALL);
    
    dict_destroy(dict);
}

TEST(DictBoundaryTest, ReplaceWithDifferentSize)
{
    dict_config_t config = {
        .capacity = 32,
        .key_type = DICT_KEY_STRING
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);
    
    // 插入小值
    int small_val = 1;
    EXPECT_EQ(dict_set(dict, "key", &small_val, sizeof(small_val)), DICT_OK);
    
    // 替换为大值
    long long large_val = 1234567890123LL;
    EXPECT_EQ(dict_set(dict, "key", &large_val, sizeof(large_val)), DICT_OK);
    
    // 验证
    long long out_val;
    EXPECT_EQ(dict_get(dict, "key", &out_val, sizeof(out_val)), DICT_OK);
    EXPECT_EQ(out_val, large_val);
    
    // 再替换为更小的值
    char tiny_val = 'x';
    EXPECT_EQ(dict_set(dict, "key", &tiny_val, sizeof(tiny_val)), DICT_OK);
    
    char out_char;
    EXPECT_EQ(dict_get(dict, "key", &out_char, sizeof(out_char)), DICT_OK);
    EXPECT_EQ(out_char, 'x');
    
    dict_destroy(dict);
}

/* ============================================
 * 测试用例：压力测试
 * ============================================ */

TEST(DictStressTest, ManyEntries)
{
    dict_config_t config = {
        .capacity = 16,
        .key_type = DICT_KEY_STRING
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);
    
    const int count = 10000;
    
    // 插入
    for (int i = 0; i < count; i++) {
        char key[32];
        snprintf(key, sizeof(key), "key_%05d", i);
        EXPECT_EQ(dict_set(dict, key, &i, sizeof(i)), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict), (size_t)count);
    
    // 验证所有
    for (int i = 0; i < count; i++) {
        char key[32];
        snprintf(key, sizeof(key), "key_%05d", i);
        int out_val;
        EXPECT_EQ(dict_get(dict, key, &out_val, sizeof(out_val)), DICT_OK);
        EXPECT_EQ(out_val, i);
    }
    
    // 删除一半
    for (int i = 0; i < count / 2; i++) {
        char key[32];
        snprintf(key, sizeof(key), "key_%05d", i);
        EXPECT_EQ(dict_delete(dict, key), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict), (size_t)(count / 2));
    
    // 验证剩余的
    for (int i = count / 2; i < count; i++) {
        char key[32];
        snprintf(key, sizeof(key), "key_%05d", i);
        int out_val;
        EXPECT_EQ(dict_get(dict, key, &out_val, sizeof(out_val)), DICT_OK);
        EXPECT_EQ(out_val, i);
    }
    
    dict_destroy(dict);
}

TEST(DictStressTest, NumberKeysHighVolume)
{
    dict_config_t config = {
        .capacity = 64,
        .key_type = DICT_KEY_NUMBER,
        .key_size = sizeof(int32_t)
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);
    
    const int count = 5000;
    
    // 插入
    for (int32_t i = 0; i < count; i++) {
        EXPECT_EQ(dict_set(dict, &i, &i, sizeof(i)), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict), (size_t)count);
    
    // 验证所有
    for (int32_t i = 0; i < count; i++) {
        int32_t out_val;
        EXPECT_EQ(dict_get(dict, &i, &out_val, sizeof(out_val)), DICT_OK);
        EXPECT_EQ(out_val, i);
    }
    
    // 随机访问
    srand(12345);
    for (int i = 0; i < 1000; i++) {
        int32_t key = rand() % count;
        int32_t out_val;
        EXPECT_EQ(dict_get(dict, &key, &out_val, sizeof(out_val)), DICT_OK);
        EXPECT_EQ(out_val, key);
    }
    
    dict_destroy(dict);
}

TEST(DictStressTest, Int64KeysHighVolume)
{
    dict_config_t config = {
        .capacity = 4096,
        .key_type = DICT_KEY_NUMBER,
        .key_size = sizeof(int64_t)
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);
    
    const int count = 500;
    
    // 插入连续的 int64 键
    for (int64_t i = 0; i < count; i++) {
        int64_t key = i;
        int val = (int)i * 10;
        EXPECT_EQ(dict_set(dict, &key, &val, sizeof(val)), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict), (size_t)count);
    
    // 验证
    for (int64_t i = 0; i < count; i++) {
        int64_t key = i;
        int out_val;
        EXPECT_EQ(dict_get(dict, &key, &out_val, sizeof(out_val)), DICT_OK);
        EXPECT_EQ(out_val, (int)i * 10);
    }
    
    dict_destroy(dict);
}

TEST(DictStressTest, ClearStress)
{
    dict_config_t config = {
        .capacity = 8,  // 小容量，强制扩容
        .key_type = DICT_KEY_STRING
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);
    
    // 多次插入和清空
    for (int round = 0; round < 10; round++) {
        for (int i = 0; i < 1000; i++) {
            char key[32];
            snprintf(key, sizeof(key), "key_r%02d_%04d", round, i);
            int val = round * 1000 + i;
            EXPECT_EQ(dict_set(dict, key, &val, sizeof(val)), DICT_OK);
        }
        EXPECT_EQ(dict_size(dict), 1000u);
        EXPECT_EQ(dict_clear(dict), DICT_OK);
        EXPECT_EQ(dict_size(dict), 0u);
    }
    
    dict_destroy(dict);
}

TEST(DictStressTest, ContinuousResize)
{
    dict_config_t config = {
        .capacity = 2,  // 极小容量，频繁扩容
        .key_type = DICT_KEY_NUMBER,
        .key_size = sizeof(int32_t)
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);
    
    const int count = 10000;
    
    // 逐步插入，每次触发扩容
    for (int32_t i = 0; i < count; i++) {
        EXPECT_EQ(dict_set(dict, &i, &i, sizeof(i)), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict), (size_t)count);
    
    // 验证
    for (int32_t i = 0; i < count; i++) {
        int32_t out_val;
        EXPECT_EQ(dict_get(dict, &i, &out_val, sizeof(out_val)), DICT_OK);
        EXPECT_EQ(out_val, i);
    }
    
    dict_destroy(dict);
}

TEST(DictStressTest, BinaryStructKeys)
{
    dict_config_t config = {
        .capacity = 64,
        .key_type = DICT_KEY_BINARY,
        .key_size = sizeof(BinaryKey)
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);
    
    const int count = 1000;
    
    // 插入结构体键
    for (int i = 0; i < count; i++) {
        BinaryKey key = {(uint16_t)(i & 0xFFFF), (uint16_t)((i >> 16) & 0xFFFF)};
        EXPECT_EQ(dict_set(dict, &key, &i, sizeof(i)), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict), (size_t)count);
    
    // 验证
    for (int i = 0; i < count; i++) {
        BinaryKey key = {(uint16_t)(i & 0xFFFF), (uint16_t)((i >> 16) & 0xFFFF)};
        int out_val;
        EXPECT_EQ(dict_get(dict, &key, &out_val, sizeof(out_val)), DICT_OK);
        EXPECT_EQ(out_val, i);
    }
    
    dict_destroy(dict);
}

TEST(DictStressTest, MixedOperations)
{
    dict_config_t config = {
        .capacity = 32,
        .key_type = DICT_KEY_STRING
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);
    
    // 混合操作：插入、查询、更新、删除
    for (int i = 0; i < 1000; i++) {
        char key[32];
        snprintf(key, sizeof(key), "key_%04d", i);
        
        // 插入
        EXPECT_EQ(dict_set(dict, key, &i, sizeof(i)), DICT_OK);
        
        // 验证
        int out_val;
        EXPECT_EQ(dict_get(dict, key, &out_val, sizeof(out_val)), DICT_OK);
        EXPECT_EQ(out_val, i);
        
        // 更新
        int new_val = i + 10000;
        EXPECT_EQ(dict_set(dict, key, &new_val, sizeof(new_val)), DICT_OK);
        
        // 验证更新
        EXPECT_EQ(dict_get(dict, key, &out_val, sizeof(out_val)), DICT_OK);
        EXPECT_EQ(out_val, new_val);
        
        // 每10个删除一个
        if (i % 10 == 0) {
            EXPECT_EQ(dict_delete(dict, key), DICT_OK);
            EXPECT_EQ(dict_get(dict, key, &out_val, sizeof(out_val)), DICT_ENOTFOUND);
        }
    }
    
    // 验证剩余的数据
    for (int i = 0; i < 1000; i++) {
        if (i % 10 == 0) continue;  // 已删除

        char key[32];
        snprintf(key, sizeof(key), "key_%04d", i);
        int out_val;
        EXPECT_EQ(dict_get(dict, key, &out_val, sizeof(out_val)), DICT_OK);
        EXPECT_EQ(out_val, i + 10000);
    }

    dict_destroy(dict);
}

/* ============================================
 * 测试用例：收缩测试
 * ============================================ */

TEST(DictShrinkTest, NullHandle)
{
    EXPECT_EQ(dict_shrink(NULL), DICT_EINVALID);
}

TEST(DictShrinkTest, EmptyDictShrink)
{
    /* 空字典收缩到默认容量 */
    dict_config_t config = {
        .capacity = 256,  /* 初始大容量 */
        .key_type = DICT_KEY_STRING
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);

    EXPECT_EQ(dict_size(dict), 0u);

    /* 收缩 */
    EXPECT_EQ(dict_shrink(dict), DICT_OK);

    dict_destroy(dict);
}

TEST(DictShrinkTest, HighLoadFactorNoShrink)
{
    /* 利用率高于25%时不收缩 */
    dict_config_t config = {
        .capacity = 32,
        .key_type = DICT_KEY_STRING
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);

    /* 插入8个元素，利用率 = 8/32 = 25%，刚好不收缩 */
    for (int i = 0; i < 8; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);
        EXPECT_EQ(dict_set(dict, key, &i, sizeof(i)), DICT_OK);
    }

    /* 收缩应该不生效（利用率刚好在阈值） */
    EXPECT_EQ(dict_shrink(dict), DICT_OK);
    EXPECT_EQ(dict_size(dict), 8u);

    dict_destroy(dict);
}

TEST(DictShrinkTest, LowLoadFactorShrink)
{
    /* 利用率低于25%时收缩 */
    dict_config_t config = {
        .capacity = 32,
        .key_type = DICT_KEY_STRING
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);

    /* 插入6个元素，利用率 = 6/32 = 18.75% < 25%，应该收缩 */
    for (int i = 0; i < 6; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);
        EXPECT_EQ(dict_set(dict, key, &i, sizeof(i)), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict), 6u);

    /* 收缩 */
    EXPECT_EQ(dict_shrink(dict), DICT_OK);
    EXPECT_EQ(dict_size(dict), 6u);  /* 大小不变 */

    /* 数据仍然正确 */
    for (int i = 0; i < 6; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);
        int out_val;
        EXPECT_EQ(dict_get(dict, key, &out_val, sizeof(out_val)), DICT_OK);
        EXPECT_EQ(out_val, i);
    }

    dict_destroy(dict);
}

TEST(DictShrinkTest, ShrinkAfterDelete)
{
    /* 删除大量数据后收缩 */
    dict_config_t config = {
        .capacity = 64,
        .key_type = DICT_KEY_STRING
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);

    /* 插入64个元素，填满容量 */
    for (int i = 0; i < 64; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);
        EXPECT_EQ(dict_set(dict, key, &i, sizeof(i)), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict), 64u);

    /* 删除60个元素，只剩4个 */
    for (int i = 0; i < 60; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);
        EXPECT_EQ(dict_delete(dict, key), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict), 4u);

    /* 收缩：利用率 = 4/64 = 6.25% < 25%，应该收缩 */
    EXPECT_EQ(dict_shrink(dict), DICT_OK);
    EXPECT_EQ(dict_size(dict), 4u);

    /* 验证剩余数据正确 */
    for (int i = 60; i < 64; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);
        int out_val;
        EXPECT_EQ(dict_get(dict, key, &out_val, sizeof(out_val)), DICT_OK);
        EXPECT_EQ(out_val, i);
    }

    dict_destroy(dict);
}

TEST(DictShrinkTest, ShrinkMinimumCapacity)
{
    /* 收缩后最小容量为32 */
    dict_config_t config = {
        .capacity = 32,
        .key_type = DICT_KEY_STRING
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);

    /* 只插入1个元素 */
    int value = 12345;
    EXPECT_EQ(dict_set(dict, "key", &value, sizeof(value)), DICT_OK);
    EXPECT_EQ(dict_size(dict), 1u);

    /* 收缩到最小容量32 */
    EXPECT_EQ(dict_shrink(dict), DICT_OK);
    EXPECT_EQ(dict_size(dict), 1u);

    /* 数据仍正确 */
    int val;
    EXPECT_EQ(dict_get(dict, "key", &val, sizeof(val)), DICT_OK);
    EXPECT_EQ(val, 12345);

    dict_destroy(dict);
}

TEST(DictShrinkTest, ShrinkAfterClear)
{
    /* 清空后收缩 */
    dict_config_t config = {
        .capacity = 128,
        .key_type = DICT_KEY_STRING
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);

    /* 插入数据 */
    for (int i = 0; i < 100; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);
        EXPECT_EQ(dict_set(dict, key, &i, sizeof(i)), DICT_OK);
    }

    /* 清空 */
    EXPECT_EQ(dict_clear(dict), DICT_OK);
    EXPECT_EQ(dict_size(dict), 0u);

    /* 收缩到默认容量32 */
    EXPECT_EQ(dict_shrink(dict), DICT_OK);
    EXPECT_EQ(dict_size(dict), 0u);

    /* 清空后仍可正常使用 */
    EXPECT_EQ(dict_set(dict, "new_key", &config, sizeof(config)), DICT_OK);
    EXPECT_EQ(dict_size(dict), 1u);

    dict_destroy(dict);
}

TEST(DictShrinkTest, NumberTypeShrink)
{
    /* NUMBER 类型键的收缩测试 */
    dict_config_t config = {
        .capacity = 64,
        .key_type = DICT_KEY_NUMBER,
        .key_size = sizeof(int32_t)
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);

    /* 插入16个元素 */
    for (int32_t i = 0; i < 16; i++) {
        EXPECT_EQ(dict_set(dict, &i, &i, sizeof(i)), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict), 16u);

    /* 删除12个，剩4个 */
    for (int32_t i = 0; i < 12; i++) {
        EXPECT_EQ(dict_delete(dict, &i), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict), 4u);

    /* 收缩 */
    EXPECT_EQ(dict_shrink(dict), DICT_OK);

    /* 验证剩余数据 */
    for (int32_t i = 12; i < 16; i++) {
        int out_val;
        EXPECT_EQ(dict_get(dict, &i, &out_val, sizeof(out_val)), DICT_OK);
        EXPECT_EQ(out_val, i);
    }

    dict_destroy(dict);
}

TEST(DictShrinkTest, MultipleShrink)
{
    /* 多次收缩测试 */
    dict_config_t config = {
        .capacity = 256,
        .key_type = DICT_KEY_STRING
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);

    /* 插入200个元素 */
    for (int i = 0; i < 200; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);
        EXPECT_EQ(dict_set(dict, key, &i, sizeof(i)), DICT_OK);
    }

    /* 分批删除并收缩 */
    for (int round = 0; round < 4; round++) {
        /* 删除50个 */
        for (int i = round * 50; i < (round + 1) * 50; i++) {
            char key[16];
            snprintf(key, sizeof(key), "key%d", i);
            EXPECT_EQ(dict_delete(dict, key), DICT_OK);
        }

        /* 收缩 */
        EXPECT_EQ(dict_shrink(dict), DICT_OK);

        /* 验证剩余数据 */
        int remaining = 200 - (round + 1) * 50;
        EXPECT_EQ(dict_size(dict), (size_t)remaining);

        for (int i = (round + 1) * 50; i < 200; i++) {
            char key[16];
            snprintf(key, sizeof(key), "key%d", i);
            int out_val;
            EXPECT_EQ(dict_get(dict, key, &out_val, sizeof(out_val)), DICT_OK);
            EXPECT_EQ(out_val, i);
        }
    }

    dict_destroy(dict);
}

TEST(DictShrinkTest, ShrinkWithResize)
{
    /* 收缩后再次扩容 */
    dict_config_t config = {
        .capacity = 16,
        .key_type = DICT_KEY_STRING
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);

    /* 插入16个触发扩容到32 */
    for (int i = 0; i < 16; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);
        EXPECT_EQ(dict_set(dict, key, &i, sizeof(i)), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict), 16u);

    /* 删除14个剩2个，触发收缩 */
    for (int i = 0; i < 14; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);
        EXPECT_EQ(dict_delete(dict, key), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict), 2u);

    /* 收缩 */
    EXPECT_EQ(dict_shrink(dict), DICT_OK);

    /* 再次插入大量数据，触发扩容 */
    for (int i = 0; i < 100; i++) {
        char key[16];
        snprintf(key, sizeof(key), "new_key%d", i);
        EXPECT_EQ(dict_set(dict, key, &i, sizeof(i)), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict), 102u);

    /* 验证所有数据 */
    for (int i = 14; i < 16; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);
        int out_val;
        EXPECT_EQ(dict_get(dict, key, &out_val, sizeof(out_val)), DICT_OK);
        EXPECT_EQ(out_val, i);
    }

    for (int i = 0; i < 100; i++) {
        char key[16];
        snprintf(key, sizeof(key), "new_key%d", i);
        int out_val;
        EXPECT_EQ(dict_get(dict, key, &out_val, sizeof(out_val)), DICT_OK);
        EXPECT_EQ(out_val, i);
    }

    dict_destroy(dict);
}

TEST(DictShrinkTest, ShrinkThenNormalUse)
{
    /* 收缩后正常写入和读取（不触发扩容） */
    dict_config_t config = {
        .capacity = 128,
        .key_type = DICT_KEY_STRING
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);

    /* 插入100个元素，触发扩容到256 */
    for (int i = 0; i < 100; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);
        EXPECT_EQ(dict_set(dict, key, &i, sizeof(i)), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict), 100u);

    /* 删除96个剩4个 (key96-key99) */
    for (int i = 0; i < 96; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);
        EXPECT_EQ(dict_delete(dict, key), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict), 4u);

    /* 收缩（利用率 4/256 = 1.56% < 25%） */
    EXPECT_EQ(dict_shrink(dict), DICT_OK);

    /* 收缩后正常读取剩余数据 */
    for (int i = 96; i < 100; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);
        int out_val;
        EXPECT_EQ(dict_get(dict, key, &out_val, sizeof(out_val)), DICT_OK);
        EXPECT_EQ(out_val, i);
    }

    /* 收缩后正常写入（只写5个，不触发扩容） */
    for (int i = 0; i < 5; i++) {
        char key[16];
        snprintf(key, sizeof(key), "new%d", i);
        int val = 100 + i;
        EXPECT_EQ(dict_set(dict, key, &val, sizeof(val)), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict), 9u);

    /* 读取所有数据验证 */
    for (int i = 96; i < 100; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);
        int out_val;
        EXPECT_EQ(dict_get(dict, key, &out_val, sizeof(out_val)), DICT_OK);
        EXPECT_EQ(out_val, i);
    }

    for (int i = 0; i < 5; i++) {
        char key[16];
        snprintf(key, sizeof(key), "new%d", i);
        int out_val;
        EXPECT_EQ(dict_get(dict, key, &out_val, sizeof(out_val)), DICT_OK);
        EXPECT_EQ(out_val, 100 + i);
    }

    /* 收缩后正常删除 */
    EXPECT_EQ(dict_delete(dict, "key96"), DICT_OK);
    EXPECT_EQ(dict_delete(dict, "new0"), DICT_OK);
    EXPECT_EQ(dict_size(dict), 7u);

    /* 删除后再次读取验证 */
    int val;
    EXPECT_EQ(dict_get(dict, "key97", &val, sizeof(val)), DICT_OK);  /* key97还在 */
    EXPECT_EQ(dict_get(dict, "key96", &val, sizeof(val)), DICT_ENOTFOUND);  /* key96已删除 */

    dict_destroy(dict);
}

TEST(DictShrinkTest, ShrinkThenUpdateAndDelete)
{
    /* 收缩后更新和删除已有数据 */
    dict_config_t config = {
        .capacity = 64,
        .key_type = DICT_KEY_STRING
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);

    /* 插入64个元素 */
    for (int i = 0; i < 64; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);
        EXPECT_EQ(dict_set(dict, key, &i, sizeof(i)), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict), 64u);

    /* 删除60个剩4个 (key60-key63) */
    for (int i = 0; i < 60; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);
        EXPECT_EQ(dict_delete(dict, key), DICT_OK);
    }
    EXPECT_EQ(dict_size(dict), 4u);

    /* 收缩 */
    EXPECT_EQ(dict_shrink(dict), DICT_OK);

    /* 收缩后更新已有数据 */
    for (int i = 60; i < 64; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);
        int new_val = i * 10;
        EXPECT_EQ(dict_set(dict, key, &new_val, sizeof(new_val)), DICT_OK);
    }

    /* 验证更新后的值 */
    for (int i = 60; i < 64; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);
        int out_val;
        EXPECT_EQ(dict_get(dict, key, &out_val, sizeof(out_val)), DICT_OK);
        EXPECT_EQ(out_val, i * 10);
    }

    /* 收缩后清空 */
    EXPECT_EQ(dict_clear(dict), DICT_OK);
    EXPECT_EQ(dict_size(dict), 0u);

    /* 清空后重新使用 */
    int value = 999;
    EXPECT_EQ(dict_set(dict, "final", &value, sizeof(value)), DICT_OK);
    EXPECT_EQ(dict_size(dict), 1u);

    int out_val;
    EXPECT_EQ(dict_get(dict, "final", &out_val, sizeof(out_val)), DICT_OK);
    EXPECT_EQ(out_val, 999);

    dict_destroy(dict);
}

/* ============================================
 * 测试用例：dict_exists
 * ============================================ */

TEST(DictExistsTest, NullHandle)
{
    EXPECT_EQ(dict_exists(NULL, "key"), 0);
}

TEST(DictExistsTest, NullKey)
{
    dict_handle_t dict = dict_create(NULL);
    ASSERT_NE(dict, nullptr);
    EXPECT_EQ(dict_exists(dict, NULL), 0);
    dict_destroy(dict);
}

TEST(DictExistsTest, KeyNotExist)
{
    dict_handle_t dict = dict_create(NULL);
    ASSERT_NE(dict, nullptr);
    EXPECT_EQ(dict_exists(dict, "nonexist"), 0);
    dict_destroy(dict);
}

TEST(DictExistsTest, KeyExists)
{
    dict_handle_t dict = dict_create(NULL);
    ASSERT_NE(dict, nullptr);

    int val = 100;
    EXPECT_EQ(dict_set(dict, "key", &val, sizeof(val)), DICT_OK);
    EXPECT_EQ(dict_exists(dict, "key"), 1);

    dict_destroy(dict);
}

TEST(DictExistsTest, AfterDelete)
{
    dict_handle_t dict = dict_create(NULL);
    ASSERT_NE(dict, nullptr);

    int val = 100;
    EXPECT_EQ(dict_set(dict, "key", &val, sizeof(val)), DICT_OK);
    EXPECT_EQ(dict_exists(dict, "key"), 1);

    EXPECT_EQ(dict_delete(dict, "key"), DICT_OK);
    EXPECT_EQ(dict_exists(dict, "key"), 0);

    dict_destroy(dict);
}

TEST(DictExistsTest, AfterClear)
{
    dict_handle_t dict = dict_create(NULL);
    ASSERT_NE(dict, nullptr);

    for (int i = 0; i < 10; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);
        EXPECT_EQ(dict_set(dict, key, &i, sizeof(i)), DICT_OK);
    }

    EXPECT_EQ(dict_clear(dict), DICT_OK);
    EXPECT_EQ(dict_exists(dict, "key0"), 0);
    EXPECT_EQ(dict_exists(dict, "key5"), 0);

    dict_destroy(dict);
}

TEST(DictExistsTest, NumberTypeKey)
{
    dict_config_t config = {
        .capacity = 32,
        .key_type = DICT_KEY_NUMBER,
        .key_size = sizeof(int32_t)
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);

    int32_t key = 12345;
    int val = 100;

    EXPECT_EQ(dict_exists(dict, &key), 0);
    EXPECT_EQ(dict_set(dict, &key, &val, sizeof(val)), DICT_OK);
    EXPECT_EQ(dict_exists(dict, &key), 1);

    dict_destroy(dict);
}

/* ============================================
 * 测试用例：dict_capacity
 * ============================================ */

TEST(DictCapacityTest, NullHandle)
{
    EXPECT_EQ(dict_capacity(NULL), 0u);
}

TEST(DictCapacityTest, DefaultCapacity)
{
    dict_handle_t dict = dict_create(NULL);
    ASSERT_NE(dict, nullptr);
    EXPECT_EQ(dict_capacity(dict), 32u);
    dict_destroy(dict);
}

TEST(DictCapacityTest, CustomCapacity)
{
    dict_config_t config = {
        .capacity = 64,
        .key_type = DICT_KEY_STRING
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);
    EXPECT_EQ(dict_capacity(dict), 64u);
    dict_destroy(dict);
}

TEST(DictCapacityTest, CapacityAfterExpand)
{
    dict_config_t config = {
        .capacity = 4,
        .key_type = DICT_KEY_STRING
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);

    EXPECT_EQ(dict_capacity(dict), 4u);

    /* 触发扩容 (负载因子0.75，容量4在插入3个时就会扩容) */
    for (int i = 0; i < 10; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);
        EXPECT_EQ(dict_set(dict, key, &i, sizeof(i)), DICT_OK);
    }

    /* 4 -> 8 -> 16 -> 32 */
    EXPECT_EQ(dict_capacity(dict), 32u);

    dict_destroy(dict);
}

TEST(DictCapacityTest, CapacityAfterShrink)
{
    dict_config_t config = {
        .capacity = 64,
        .key_type = DICT_KEY_STRING
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);

    EXPECT_EQ(dict_capacity(dict), 64u);

    /* 插入4个元素 */
    for (int i = 0; i < 4; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);
        EXPECT_EQ(dict_set(dict, key, &i, sizeof(i)), DICT_OK);
    }

    /* 删除触发收缩 */
    EXPECT_EQ(dict_delete(dict, "key0"), DICT_OK);
    EXPECT_EQ(dict_delete(dict, "key1"), DICT_OK);
    EXPECT_EQ(dict_delete(dict, "key2"), DICT_OK);
    EXPECT_EQ(dict_delete(dict, "key3"), DICT_OK);

    /* 收缩到默认容量32 */
    EXPECT_EQ(dict_shrink(dict), DICT_OK);
    EXPECT_EQ(dict_capacity(dict), 32u);

    dict_destroy(dict);
}

TEST(DictCapacityTest, PowerOfTwoAlignment)
{
    /* 非2的幂次应该对齐到2的幂次 */
    dict_config_t config = {
        .capacity = 10,
        .key_type = DICT_KEY_STRING
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);
    EXPECT_EQ(dict_capacity(dict), 16u);
    dict_destroy(dict);
}

TEST(DictCapacityTest, NumberTypeKey)
{
    dict_config_t config = {
        .capacity = 128,
        .key_type = DICT_KEY_NUMBER,
        .key_size = sizeof(int64_t)
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);
    EXPECT_EQ(dict_capacity(dict), 128u);
    dict_destroy(dict);
}

/* ============================================
 * 迭代器测试
 * ============================================ */

TEST(DictIteratorTest, CreateAndDestroy)
{
    dict_handle_t dict = dict_create(NULL);
    ASSERT_NE(dict, nullptr);

    /* 空字典创建迭代器 */
    dict_iter_t iter = dict_iter_create(dict);
    EXPECT_NE(iter, nullptr);

    /* 空字典没有数据，get 返回未找到 */
    EXPECT_EQ(dict_iter_get(iter, NULL, NULL, NULL, NULL), DICT_ENOTFOUND);

    dict_iter_destroy(iter);
    dict_destroy(dict);
}

TEST(DictIteratorTest, EmptyDict)
{
    dict_handle_t dict = dict_create(NULL);
    ASSERT_NE(dict, nullptr);

    dict_iter_t iter = dict_iter_create(dict);
    ASSERT_NE(iter, nullptr);

    /* 迭代器创建后指向第一个元素（但没有元素），get 返回未找到 */
    char key[64];
    size_t klen;
    int value = 0;
    size_t vlen;
    EXPECT_EQ(dict_iter_get(iter, key, &klen, &value, &vlen), DICT_ENOTFOUND);

    dict_iter_destroy(iter);
    dict_destroy(dict);
}

TEST(DictIteratorTest, BasicTraversal)
{
    dict_handle_t dict = dict_create(NULL);
    ASSERT_NE(dict, nullptr);

    /* 插入3个元素 */
    ASSERT_EQ(dict_set(dict, "key1", "value1", 6), DICT_OK);
    ASSERT_EQ(dict_set(dict, "key2", "value2", 6), DICT_OK);
    ASSERT_EQ(dict_set(dict, "key3", "value3", 6), DICT_OK);

    /* 遍历计数 */
    int count = 0;
    char key[64];
    char value[64];
    size_t klen, vlen;

    dict_iter_t iter = dict_iter_create(dict);
    ASSERT_NE(iter, nullptr);

    while (dict_iter_get(iter, key, &klen, value, &vlen) == DICT_OK) {
        count++;
        EXPECT_LE(klen, (size_t)63);
        EXPECT_LE(vlen, (size_t)63);
        dict_iter_next(iter);
    }

    EXPECT_EQ(count, 3);

    dict_iter_destroy(iter);
    dict_destroy(dict);
}

TEST(DictIteratorTest, AllElementsTraversed)
{
    dict_handle_t dict = dict_create(NULL);
    ASSERT_NE(dict, nullptr);

    /* 插入多个元素 */
    const int COUNT = 10;
    for (int i = 0; i < COUNT; i++) {
        char key[16];
        char value[16];
        snprintf(key, sizeof(key), "key%d", i);
        snprintf(value, sizeof(value), "val%d", i);
        ASSERT_EQ(dict_set(dict, key, value, (size_t)(value[3] == '\0' ? 4 : strlen(value))), DICT_OK);
    }

    /* 记录遍历到的键 */
    bool found[COUNT] = {false};
    int count = 0;

    char key[16];
    size_t klen;

    dict_iter_t iter = dict_iter_create(dict);
    ASSERT_NE(iter, nullptr);

    while (dict_iter_get(iter, key, &klen, NULL, NULL) == DICT_OK) {
        /* 解析键编号 */
        if (klen >= 4 && key[0] == 'k' && key[1] == 'e' && key[2] == 'y') {
            int idx = atoi(key + 3);
            if (idx >= 0 && idx < COUNT) {
                EXPECT_FALSE(found[idx]);  // 不应该重复
                found[idx] = true;
                count++;
            }
        }
        dict_iter_next(iter);
    }

    EXPECT_EQ(count, COUNT);
    for (int i = 0; i < COUNT; i++) {
        EXPECT_TRUE(found[i]);  // 所有元素都应该被遍历到
    }

    dict_iter_destroy(iter);
    dict_destroy(dict);
}

TEST(DictIteratorTest, NullOutputParams)
{
    dict_handle_t dict = dict_create(NULL);
    ASSERT_NE(dict, nullptr);

    ASSERT_EQ(dict_set(dict, "key1", "value1", 6), DICT_OK);

    dict_iter_t iter = dict_iter_create(dict);
    ASSERT_NE(iter, nullptr);

    /* 测试各种 NULL 参数组合 */
    EXPECT_EQ(dict_iter_get(iter, NULL, NULL, NULL, NULL), DICT_OK);
    dict_iter_next(iter);  /* 移动到下一个 */
    EXPECT_EQ(dict_iter_get(iter, NULL, NULL, NULL, NULL), DICT_ENOTFOUND);

    dict_iter_destroy(iter);
    dict_destroy(dict);
}

TEST(DictIteratorTest, OnlyKeyOrOnlyValue)
{
    dict_handle_t dict = dict_create(NULL);
    ASSERT_NE(dict, nullptr);

    ASSERT_EQ(dict_set(dict, "test_key", "test_value", 10), DICT_OK);

    dict_iter_t iter = dict_iter_create(dict);
    ASSERT_NE(iter, nullptr);

    /* 只获取键 */
    char key[64];
    size_t klen;
    memset(key, 'X', sizeof(key));  // 填充非零值
    EXPECT_EQ(dict_iter_get(iter, key, &klen, NULL, NULL), DICT_OK);
    EXPECT_EQ(memcmp(key, "test_key", 8), 0);
    EXPECT_EQ(klen, (size_t)8);

    /* 只获取值 */
    char value[64];
    size_t vlen;
    memset(value, 'X', sizeof(value));
    EXPECT_EQ(dict_iter_get(iter, NULL, NULL, value, &vlen), DICT_OK);
    EXPECT_EQ(memcmp(value, "test_value", 10), 0);
    EXPECT_EQ(vlen, (size_t)10);

    dict_iter_destroy(iter);
    dict_destroy(dict);
}

TEST(DictIteratorTest, NestedIteration)
{
    dict_handle_t dict1 = dict_create(NULL);
    dict_handle_t dict2 = dict_create(NULL);
    ASSERT_NE(dict1, nullptr);
    ASSERT_NE(dict2, nullptr);

    /* 填充两个字典 */
    ASSERT_EQ(dict_set(dict1, "a", "1", 1), DICT_OK);
    ASSERT_EQ(dict_set(dict1, "b", "2", 1), DICT_OK);
    ASSERT_EQ(dict_set(dict2, "c", "3", 1), DICT_OK);
    ASSERT_EQ(dict_set(dict2, "d", "4", 1), DICT_OK);

    /* 同时遍历两个字典 */
    int count1 = 0, count2 = 0;
    dict_iter_t iter1 = dict_iter_create(dict1);
    dict_iter_t iter2 = dict_iter_create(dict2);
    ASSERT_NE(iter1, nullptr);
    ASSERT_NE(iter2, nullptr);

    while (dict_iter_get(iter1, NULL, NULL, NULL, NULL) == DICT_OK) {
        count1++;
        dict_iter_next(iter1);
    }

    while (dict_iter_get(iter2, NULL, NULL, NULL, NULL) == DICT_OK) {
        count2++;
        dict_iter_next(iter2);
    }

    EXPECT_EQ(count1, 2);
    EXPECT_EQ(count2, 2);

    dict_iter_destroy(iter1);
    dict_iter_destroy(iter2);
    dict_destroy(dict1);
    dict_destroy(dict2);
}

TEST(DictIteratorTest, NullHandle)
{
    /* NULL 句柄 */
    EXPECT_EQ(dict_iter_create(NULL), nullptr);
    EXPECT_EQ(dict_iter_get(NULL, NULL, NULL, NULL, NULL), DICT_EINVALID);
    dict_iter_next(NULL);  /* 应该安全处理 */
    dict_iter_destroy(NULL);  /* 应该安全处理 */
}

TEST(DictIteratorTest, ConditionDelete)
{
    dict_handle_t dict = dict_create(NULL);
    ASSERT_NE(dict, nullptr);

    /* 插入10个整数，5个正数5个负数 */
    for (int i = 0; i < 10; i++) {
        char key[16];
        int value = (i < 5) ? i : i - 10;  // 0,1,2,3,4,-5,-4,-3,-2,-1
        snprintf(key, sizeof(key), "key%d", i);
        ASSERT_EQ(dict_set(dict, key, &value, sizeof(value)), DICT_OK);
    }

    EXPECT_EQ(dict_size(dict), 10u);

    /* 删除所有负数 */
    char key[16];
    int value;
    size_t klen, vlen;

    dict_iter_t iter = dict_iter_create(dict);
    ASSERT_NE(iter, nullptr);

    while (dict_iter_get(iter, key, &klen, &value, &vlen) == DICT_OK) {
        if (vlen == sizeof(int) && value < 0) {
            dict_delete(dict, key);
            /* 删除后需要重新开始迭代 */
            dict_iter_destroy(iter);
            iter = dict_iter_create(dict);
            if (!iter) break;
        } else {
            dict_iter_next(iter);
        }
    }

    dict_iter_destroy(iter);

    /* 只剩5个正数 */
    EXPECT_EQ(dict_size(dict), 5u);

    dict_destroy(dict);
}

TEST(DictIteratorTest, NumberKeyType)
{
    dict_config_t config = {
        .capacity = 32,
        .key_type = DICT_KEY_NUMBER,
        .key_size = sizeof(int32_t),
        .hash_fn = NULL
    };
    dict_handle_t dict = dict_create(&config);
    ASSERT_NE(dict, nullptr);

    /* 插入整数键值对 */
    for (int i = 0; i < 5; i++) {
        int32_t key = i * 100;
        char value[16];
        snprintf(value, sizeof(value), "val%d", i);
        ASSERT_EQ(dict_set(dict, &key, value, (size_t)(strlen(value) + 1)), DICT_OK);
    }

    /* 遍历 */
    int count = 0;
    dict_iter_t iter = dict_iter_create(dict);
    ASSERT_NE(iter, nullptr);

    while (dict_iter_get(iter, NULL, NULL, NULL, NULL) == DICT_OK) {
        count++;
        dict_iter_next(iter);
    }

    EXPECT_EQ(count, 5);

    dict_iter_destroy(iter);
    dict_destroy(dict);
}

