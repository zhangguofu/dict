# 通用字典模块

C语言通用字典模块，支持任意数据类型的键值对存取。

## 1. 特性

- **多类型支持**：STRING、NUMBER(int/uint)、BINARY 三种键类型
- **自动内存管理**：用户无需关心键值生命周期
- **整数哈希优化**：NUMBER 类型使用专用哈希函数，比 MurmurHash3 快 4.7x
- **跨平台支持**：专为 RT-Thread 设计，代码也可运行于 PC 端
- **嵌入式友好**：代码量小（~600行）、内存占用可控
- **迭代器支持**：安全遍历所有键值对

## 2. 快速开始

### 2.1 创建字典

```c
#include "dict.h"

dict_config_t config = {
    .capacity = 64,
    .key_type = DICT_KEY_STRING
};
dict_handle_t dict = dict_create(&config);
```

### 2.2 存储数据

```c
// 存储字符串值
const char *json_data = "{\"temp\":25.5}";
dict_set(dict, "sensor_1", json_data, strlen(json_data) + 1);

// 存储整数
int temp = 25;
dict_set(dict, "temperature", &temp, sizeof(temp));
```

### 2.3 获取数据

```c
char buf[256];
dict_get(dict, "sensor_1", buf, sizeof(buf));

int temp_out;
dict_get(dict, "temperature", &temp_out, sizeof(temp_out));
```

### 2.4 遍历字典

```c
dict_iter_t iter = dict_iter_create(dict);
char key[64];
char value[256];
size_t klen, vlen;

while (dict_iter_get(iter, key, &klen, value, &vlen) == DICT_OK) {
    printf("%s = %s\n", key, value);
    dict_iter_next(iter);
}
dict_iter_destroy(iter);
```

### 2.5 销毁字典

```c
dict_destroy(dict);
```

## 3. 构建与测试

```bash
# 清理并编译（默认启用 ASAN 内存泄漏检测）
scons -c && scons

# 运行所有测试
./build/bin/dict_test

# 运行基准测试
./build/bin/dict_benchmark

```

## 4. 项目结构

```
dict/
├── inc/
│   └── dict.h          # 公共API + 平台抽象
└── src/
    ├── dict_core.c     # 核心实现
    ├── dict_core.h     # 内部数据结构
    ├── dict_hash.c     # 哈希算法
    └── dict_hash.h     # 哈希算法内部头文件

docs/
├── requirements.md     # 需求规格
├── design.md           # 设计文档（包含迭代器设计）
└── api.md              # API 参考
```

## 5. 使用示例（斐波那契数列）

```c

#include "dict.h"


static dict_handle_t fib_get_cache(void)
{
    static dict_handle_t fib_cache = NULL;
    if (NULL == fib_cache) {
        dict_config_t config = {
            .capacity = 128,
            .key_type = DICT_KEY_NUMBER,
            .key_size = sizeof(int)
        };
        fib_cache = dict_create(&config);
    }

    return fib_cache;
}

uint64_t fib_memoized(int n) {

    if (n <= 1) {
        return n;
    }

    uint64_t value = 0;
    dict_handle_t cache = fib_get_cache();
    if (DICT_OK == dict_get(cache, &n, &value, sizeof(value))) {
        return value;
    }

    value = fib_memoized(n - 1) + fib_memoized(n - 2);
    dict_set(cache, key, &value, sizeof(value));
    return value;
}

int main(void)
{
    int test_cases[] = {20, 25, 30, 40, 50, 60, 90};
    size_t test_count = sizeof(test_cases) / sizeof(test_cases[0]);

    for (size_t i = 0; i < test_count; i++) {
        uint64_t value = fib_memoized(test_cases[i]);
        printf("Fib(%d) = %llu\n", test_cases[i], value);
    }
}

```
