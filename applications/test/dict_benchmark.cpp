/**
 * @file dict_benchmark.cpp
 * @brief 字典性能基准测试
 */

#include "dict.h"
#include "dict_hash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

// ============================================
// 计时器封装
// ============================================

static double get_time_ms(void)
{
#ifdef __APPLE__
    static mach_timebase_info_data_t info = {0, 0};
    if (info.denom == 0) {
        mach_timebase_info(&info);
    }
    uint64_t elapsed = mach_absolute_time();
    return (double)elapsed * info.numer / info.denom / 1000000.0;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
#endif
}

// ============================================
// 测试配置
// ============================================

#define BENCH_ITERATIONS 10000
#define BENCH_DICT_SIZE  1000

// ============================================
// 基准测试辅助函数
// ============================================

typedef struct {
    const char *name;
    double elapsed;
    int iterations;
} bench_result_t;

static void print_bench_result(const char *name, double elapsed_ms, int iterations)
{
    double avg_us = elapsed_ms * 1000.0 / iterations;
    printf("%-45s %8.2f ms / %d ops  (%6.2f us/op)\n", name, elapsed_ms, iterations, avg_us);
}

// ============================================
// STRING 类型测试
// ============================================

static void benchmark_string_insert(void)
{
    dict_config_t config = {
        .capacity = BENCH_DICT_SIZE * 2,
        .key_type = DICT_KEY_STRING
    };
    dict_handle_t dict = dict_create(&config);
    
    char key[32];
    char value[64];
    
    double start = get_time_ms();
    for (int i = 0; i < BENCH_ITERATIONS; i++) {
        snprintf(key, sizeof(key), "key_%08d", i);
        snprintf(value, sizeof(value), "value_%08d", i);
        dict_set(dict, key, value, strlen(value) + 1);
    }
    double elapsed = get_time_ms() - start;
    
    print_bench_result("STRING: Insert (10k iterations)", elapsed, BENCH_ITERATIONS);
    dict_destroy(dict);
}

static void benchmark_string_lookup(void)
{
    dict_config_t config = {
        .capacity = BENCH_DICT_SIZE * 2,
        .key_type = DICT_KEY_STRING
    };
    dict_handle_t dict = dict_create(&config);
    
    char key[32];
    char value[64];
    for (int i = 0; i < BENCH_DICT_SIZE; i++) {
        snprintf(key, sizeof(key), "key_%08d", i);
        snprintf(value, sizeof(value), "value_%08d", i);
        dict_set(dict, key, value, strlen(value) + 1);
    }
    
    char query[32];
    char result[64];
    
    double start = get_time_ms();
    for (int i = 0; i < BENCH_ITERATIONS; i++) {
        snprintf(query, sizeof(query), "key_%08d", i % BENCH_DICT_SIZE);
        dict_get(dict, query, result, sizeof(result));
    }
    double elapsed = get_time_ms() - start;
    
    print_bench_result("STRING: Lookup (10k iterations)", elapsed, BENCH_ITERATIONS);
    dict_destroy(dict);
}

static void benchmark_string_delete(void)
{
    dict_config_t config = {
        .capacity = BENCH_DICT_SIZE * 2,
        .key_type = DICT_KEY_STRING
    };
    dict_handle_t dict = dict_create(&config);
    
    char key[32];
    char value[64];
    for (int i = 0; i < BENCH_ITERATIONS; i++) {
        snprintf(key, sizeof(key), "key_%08d", i);
        snprintf(value, sizeof(value), "value_%08d", i);
        dict_set(dict, key, value, strlen(value) + 1);
    }
    
    char query[32];
    
    double start = get_time_ms();
    for (int i = 0; i < BENCH_ITERATIONS; i++) {
        snprintf(query, sizeof(query), "key_%08d", i);
        dict_delete(dict, query);
    }
    double elapsed = get_time_ms() - start;
    
    print_bench_result("STRING: Delete (10k iterations)", elapsed, BENCH_ITERATIONS);
    dict_destroy(dict);
}

// ============================================
// NUMBER 类型测试 (int32)
// ============================================

static void benchmark_number_insert(void)
{
    dict_config_t config = {
        .capacity = BENCH_DICT_SIZE * 2,
        .key_type = DICT_KEY_NUMBER,
        .key_size = sizeof(int32_t)
    };
    dict_handle_t dict = dict_create(&config);
    
    int32_t key;
    char value[64];
    
    double start = get_time_ms();
    for (int i = 0; i < BENCH_ITERATIONS; i++) {
        key = i;
        snprintf(value, sizeof(value), "value_%08d", i);
        dict_set(dict, &key, value, strlen(value) + 1);
    }
    double elapsed = get_time_ms() - start;
    
    print_bench_result("NUMBER(int32): Insert (10k iterations)", elapsed, BENCH_ITERATIONS);
    dict_destroy(dict);
}

static void benchmark_number_lookup(void)
{
    dict_config_t config = {
        .capacity = BENCH_DICT_SIZE * 2,
        .key_type = DICT_KEY_NUMBER,
        .key_size = sizeof(int32_t)
    };
    dict_handle_t dict = dict_create(&config);
    
    int32_t key;
    char value[64];
    for (int i = 0; i < BENCH_DICT_SIZE; i++) {
        key = i;
        snprintf(value, sizeof(value), "value_%08d", i);
        dict_set(dict, &key, value, strlen(value) + 1);
    }
    
    int32_t query;
    char result[64];
    
    double start = get_time_ms();
    for (int i = 0; i < BENCH_ITERATIONS; i++) {
        query = i % BENCH_DICT_SIZE;
        dict_get(dict, &query, result, sizeof(result));
    }
    double elapsed = get_time_ms() - start;
    
    print_bench_result("NUMBER(int32): Lookup (10k iterations)", elapsed, BENCH_ITERATIONS);
    dict_destroy(dict);
}

static void benchmark_number_delete(void)
{
    dict_config_t config = {
        .capacity = BENCH_DICT_SIZE * 2,
        .key_type = DICT_KEY_NUMBER,
        .key_size = sizeof(int32_t)
    };
    dict_handle_t dict = dict_create(&config);
    
    int32_t key;
    char value[64];
    for (int i = 0; i < BENCH_ITERATIONS; i++) {
        key = i;
        snprintf(value, sizeof(value), "value_%08d", i);
        dict_set(dict, &key, value, strlen(value) + 1);
    }
    
    double start = get_time_ms();
    for (int i = 0; i < BENCH_ITERATIONS; i++) {
        key = i;
        dict_delete(dict, &key);
    }
    double elapsed = get_time_ms() - start;
    
    print_bench_result("NUMBER(int32): Delete (10k iterations)", elapsed, BENCH_ITERATIONS);
    dict_destroy(dict);
}

// ============================================
// NUMBER 类型测试 (int64)
// ============================================

static void benchmark_number64_insert(void)
{
    dict_config_t config = {
        .capacity = BENCH_DICT_SIZE * 2,
        .key_type = DICT_KEY_NUMBER,
        .key_size = sizeof(int64_t)
    };
    dict_handle_t dict = dict_create(&config);
    
    int64_t key;
    char value[64];
    
    double start = get_time_ms();
    for (int i = 0; i < BENCH_ITERATIONS; i++) {
        key = (int64_t)i * 1000000;
        snprintf(value, sizeof(value), "value_%08d", i);
        dict_set(dict, &key, value, strlen(value) + 1);
    }
    double elapsed = get_time_ms() - start;
    
    print_bench_result("NUMBER(int64): Insert (10k iterations)", elapsed, BENCH_ITERATIONS);
    dict_destroy(dict);
}

static void benchmark_number64_lookup(void)
{
    dict_config_t config = {
        .capacity = BENCH_DICT_SIZE * 2,
        .key_type = DICT_KEY_NUMBER,
        .key_size = sizeof(int64_t)
    };
    dict_handle_t dict = dict_create(&config);
    
    int64_t key;
    char value[64];
    for (int i = 0; i < BENCH_DICT_SIZE; i++) {
        key = (int64_t)i * 1000000;
        snprintf(value, sizeof(value), "value_%08d", i);
        dict_set(dict, &key, value, strlen(value) + 1);
    }
    
    int64_t query;
    char result[64];
    
    double start = get_time_ms();
    for (int i = 0; i < BENCH_ITERATIONS; i++) {
        query = (int64_t)(i % BENCH_DICT_SIZE) * 1000000;
        dict_get(dict, &query, result, sizeof(result));
    }
    double elapsed = get_time_ms() - start;
    
    print_bench_result("NUMBER(int64): Lookup (10k iterations)", elapsed, BENCH_ITERATIONS);
    dict_destroy(dict);
}

// ============================================
// 扩容性能测试
// ============================================

static void benchmark_resize(void)
{
    dict_config_t config = {
        .capacity = 8,
        .key_type = DICT_KEY_NUMBER,
        .key_size = sizeof(int32_t)
    };
    dict_handle_t dict = dict_create(&config);
    
    int32_t key;
    char value[64];
    
    double start = get_time_ms();
    for (int i = 0; i < 1000; i++) {
        key = i;
        snprintf(value, sizeof(value), "value_%08d", i);
        dict_set(dict, &key, value, strlen(value) + 1);
    }
    double elapsed = get_time_ms() - start;
    
    print_bench_result("Resize: Insert with rehashing (1k)", elapsed, 1000);
    printf("  Final dict size: %zu\n", dict_size(dict));
    
    dict_destroy(dict);
}

// ============================================
// 哈希冲突测试
// ============================================

static uint32_t collision_hash(const void *key, size_t len)
{
    (void)key; (void)len;
    return 0;
}

static void benchmark_collision(void)
{
    dict_config_t config = {
        .capacity = 8,
        .key_type = DICT_KEY_STRING,
        .hash_fn = collision_hash
    };
    dict_handle_t dict = dict_create(&config);
    
    char key[32];
    char value[64];
    
    double start = get_time_ms();
    for (int i = 0; i < 1000; i++) {
        snprintf(key, sizeof(key), "key_%08d", i);
        snprintf(value, sizeof(value), "value_%08d", i);
        dict_set(dict, key, value, strlen(value) + 1);
    }
    double elapsed = get_time_ms() - start;
    
    print_bench_result("Collision: Insert (1k, worst case)", elapsed, 1000);
    
    char result[64];
    start = get_time_ms();
    for (int i = 0; i < 1000; i++) {
        snprintf(key, sizeof(key), "key_%08d", i);
        dict_get(dict, key, result, sizeof(result));
    }
    elapsed = get_time_ms() - start;
    
    print_bench_result("Collision: Lookup (1k, worst case)", elapsed, 1000);
    
    dict_destroy(dict);
}

// ============================================
// 哈希函数性能对比
// ============================================

static void benchmark_hash_functions(void)
{
    char key[32] = "test_key_string_12345";
    size_t key_len = strlen(key);
    double start, end;
    
    start = get_time_ms();
    for (int i = 0; i < 100000; i++) {
        murmur3_32(key, key_len);
    }
    end = get_time_ms();
    print_bench_result("MurmurHash3 (20B string)", (end - start), 100000);
    
    int32_t int_key = 12345;
    start = get_time_ms();
    for (int i = 0; i < 100000; i++) {
        dict_number_hash(&int_key, sizeof(int_key));
    }
    end = get_time_ms();
    print_bench_result("dict_number_hash (int32)", (end - start), 100000);
    
    int64_t int64_key = 12345;
    start = get_time_ms();
    for (int i = 0; i < 100000; i++) {
        dict_number_hash(&int64_key, sizeof(int64_key));
    }
    end = get_time_ms();
    print_bench_result("dict_number_hash (int64)", (end - start), 100000);
}

// ============================================
// 主函数
// ============================================

int main(void)
{
    printf("===========================================================\n");
    printf("             Dictionary Benchmark Suite\n");
    printf("===========================================================\n");
    printf("Configuration:\n");
    printf("  - Iterations: %d\n", BENCH_ITERATIONS);
    printf("  - Dict size: %d\n", BENCH_DICT_SIZE);
    printf("===========================================================\n\n");
    
    printf("%-45s %18s\n", "Test", "Result");
    printf("%-45s %18s\n", "----", "------");
    
    printf("\n%-45s\n", "=== STRING Key Tests ===");
    benchmark_string_insert();
    benchmark_string_lookup();
    benchmark_string_delete();
    
    printf("\n%-45s\n", "=== NUMBER Key Tests (int32) ===");
    benchmark_number_insert();
    benchmark_number_lookup();
    benchmark_number_delete();
    
    printf("\n%-45s\n", "=== NUMBER Key Tests (int64) ===");
    benchmark_number64_insert();
    benchmark_number64_lookup();
    
    printf("\n");
    benchmark_resize();
    
    printf("\n%-45s\n", "=== Hash Collision Test (all keys -> same bucket) ===");
    benchmark_collision();
    
    printf("\n%-45s\n", "=== Hash Function Comparison (100k iterations) ===");
    benchmark_hash_functions();
    
    printf("\n===========================================================\n");
    printf("                    Benchmark Complete\n");
    printf("===========================================================\n");
    
    return 0;
}
