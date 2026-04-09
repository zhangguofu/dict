# 通用字典模块

C语言通用字典模块，支持任意数据类型的键值对存取。

## 1. 特性

- **多类型支持**：STRING、NUMBER(int/uint)、BINARY 三种键类型
- **自动内存管理**：用户无需关心键值生命周期
- **高效哈希算法**：采用 MurmurHash3（效率高，分布均匀，抗碰撞能力强），对 NUMBER 类型整数键进一步优化，效率更高
- **跨平台支持**：专为 RT-Thread 设计，代码也可运行于 PC 端
- **嵌入式友好**：代码量小（~600行）、内存占用可控
- **迭代器支持**：安全遍历所有键值对

## 2. API 列表

### 2.1 基础操作

| 函数 | 说明 |
|------|------|
| `dict_create(config)` | 创建字典 |
| `dict_clear(handle)` | 清空所有键值对 |
| `dict_size(handle)` | 获取元素数量 |
| `dict_capacity(handle)` | 获取当前容量 |
| `dict_destroy(handle)` | 销毁字典，释放所有内存 |

### 2.2 键值操作

| 函数 | 说明 |
|------|------|
| `dict_set(handle, key, value, len)` | 设置键值对（插入或更新） |
| `dict_get(handle, key, buf, len)` | 获取值，buf 为 NULL 时只检查键是否存在 |
| `dict_get_size(handle, key, size_out)` | 获取值的数据大小 |
| `dict_delete(handle, key)` | 删除键值对 |
| `dict_exists(handle, key)` | 检查键是否存在 |

### 2.3 内存优化

| 函数 | 说明 |
|------|------|
| `dict_shrink(handle)` | 收缩容量至最小 2^n，释放多余内存 |

### 2.4 迭代器

| 函数 | 说明 |
|------|------|
| `dict_iter_create(handle)` | 创建迭代器 |
| `dict_iter_get(iter, ...)` | 获取当前元素（键、值、长度） |
| `dict_iter_next(iter)` | 移动到下一个元素 |
| `dict_iter_destroy(iter)` | 销毁迭代器 |


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
# 进入应用目录
cd applications

# 清理并编译（默认启用 ASAN 内存泄漏检测）
scons -c && scons

# 运行所有测试
./build/bin/dict_test

# 运行基准测试
./build/bin/dict_benchmark

# 运行示例程序
./build/bin/fibonacci
```

## 4. 项目结构

```
├── SConscript           # RT-Thread 软件包构建配置
├── inc/
│   └── dict.h          # 公共API + 平台抽象
├── src/
│   ├── dict_core.c     # 核心实现
│   ├── dict_core.h     # 内部数据结构
│   ├── dict_hash.c     # 哈希算法
│   └── dict_hash.h     # 哈希算法内部头文件
├── applications/       # PC 端测试与示例（独立构建）
│   ├── SConstruct      # 构建入口
│   ├── test/           # 单元测试
│   └── examples/        # 使用示例
└── README.md
```

## 5. RT-Thread 集成

本模块可作为 RT-Thread 软件包使用。集成到 RT-Thread 项目时：

1. 在 RT-Thread Studio 或使用 `menuconfig` 启用 `dict` 软件包
2. 软件包会自动编译，无需额外配置

## 6. 使用示例（斐波那契数列）

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

uint64_t fib_memoized(int n)
{

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
