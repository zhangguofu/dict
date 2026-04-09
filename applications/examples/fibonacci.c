/**
 * @file fibonacci.c
 * @brief Fibonacci calculation - using dictionary optimization (memoization)
 *
 * Demonstrates how to use dict library to implement memoized recursion,
 * converting exponential time complexity O(2^n) to linear O(n).
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include "dict.h"

/* ============================================
 * Method 1: Naive recursion (exponential time complexity O(2^n))
 * ============================================ */

uint64_t fib_naive(int n)
{
    if (n <= 1) {
        return n;
    }
    return fib_naive(n - 1) + fib_naive(n - 2);
}

/* ============================================
 * Method 2: Memoized recursion (linear time complexity O(n))
 * Uses dict to cache computed results
 * ============================================ */

static dict_handle_t g_fib_cache = NULL;

/* Global cache dictionary (lazy initialization) */
static dict_handle_t fib_get_cache(void)
{
    if (g_fib_cache == NULL) {
        dict_config_t config = {
            .capacity = 128,
            .key_type = DICT_KEY_NUMBER,
            .key_size = sizeof(int)
        };
        g_fib_cache = dict_create(&config);
    }
    return g_fib_cache;
}

uint64_t fib_memoized(int n)
{
    dict_handle_t cache = fib_get_cache();
    uint64_t result;

    /* 1. Check if result is already in cache */
    if (dict_get(cache, &n, &result, sizeof(result)) == DICT_OK) {
        return result;  /* Cache hit, return directly */
    }

    /* 2. Base case */
    if (n <= 1) {
        return (uint64_t)n;
    }

    /* 3. Recursive computation */
    result = (uint64_t)fib_memoized(n - 1) + (uint64_t)fib_memoized(n - 2);

    /* 4. Store result in cache */
    dict_set(cache, &n, &result, sizeof(result));

    return result;
}

/* ============================================
 * Test case definitions
 * ============================================ */

typedef struct {
    int n;
    uint64_t expected;
} fib_test_case_t;

/* Fibonacci reference values */
static const fib_test_case_t g_test_cases[] = {
    {0,  0},
    {1,  1},
    {2,  1},
    {3,  2},
    {4,  3},
    {5,  5},
    {6,  8},
    {7,  13},
    {8,  21},
    {9,  34},
    {10, 55},
    {20, 6765},
    {30, 832040},
    {40, 102334155},
    {92, 7540113804746346429ULL},  /* F(92) is the maximum value representable by uint64_t */
};

static int g_test_count = sizeof(g_test_cases) / sizeof(g_test_cases[0]);

/* ============================================
 * Correctness verification
 * ============================================ */

static int verify_results(void)
{
    printf("\n========== 正确性验证 ==========\n\n");

    int passed = 0;
    for (int i = 0; i < g_test_count; i++) {
        int n = g_test_cases[i].n;
        uint64_t expected = g_test_cases[i].expected;

        uint64_t result = fib_memoized(n);

        if (result == expected) {
            printf("  F(%02d) = %20llu  [通过]\n", n, (unsigned long long)result);
            passed++;
        } else {
            printf("  F(%02d) = %20llu (预期 %20llu)  [失败]\n",
                   n, (unsigned long long)result, (unsigned long long)expected);
        }
    }

    printf("\n验证结果: %d/%d 通过\n", passed, g_test_count);
    return passed == g_test_count ? 0 : 1;
}

/* ============================================
 * Performance comparison
 * ============================================ */

static void performance_comparison(void)
{
    printf("\n========== 性能对比 ==========\n\n");

    int test_cases[] = {20, 25, 30, 35, 40};
    size_t test_count = sizeof(test_cases) / sizeof(test_cases[0]);

    for (size_t i = 0; i < test_count; i++) {
        int n = test_cases[i];
        clock_t start, end;
        double naive_ms, memo_ms;

        printf("[n = %d]\n", n);

        /* Naive recursion: single timing */
        start = clock();
        volatile uint64_t r1 = fib_naive(n);
        end = clock();
        naive_ms = (double)(end - start) / CLOCKS_PER_SEC * 1000;
        printf("  普通递归:   F(%2d) = %llu, 耗时: %.2f ms\n",
               n, (unsigned long long)r1, naive_ms);

        /* Memoized recursion: time 1000 times, then calculate average */
        start = clock();
        for (int j = 0; j < 1000; j++) {
            volatile uint64_t r2 = fib_memoized(n);
            (void)r2;
        }
        end = clock();
        memo_ms = (double)(end - start) / CLOCKS_PER_SEC * 1000 / 1000;
        printf("  记忆化:     F(%2d) = %llu, 耗时: %.4f ms (1000次平均)\n",
               n, (unsigned long long)fib_memoized(n), memo_ms);

        /* Speedup ratio */
        printf("  加速比:     %.0fx\n\n", naive_ms / memo_ms + 0.5);
    }
}

/* ============================================
 * Cache statistics
 * ============================================ */

static void print_cache_stats(void)
{
    dict_handle_t cache = fib_get_cache();
    printf("========== 缓存统计 ==========\n\n");
    printf("  缓存条目数: %zu\n", dict_size(cache));
    printf("  键类型: NUMBER (int)\n");
}

/* ============================================
 * Main function
 * ============================================ */

int main(void)
{
    printf("============================================================\n");
    printf("       斐波那契数列 - 字典记忆化示例                        \n");
    printf("============================================================\n\n");

    printf("本程序演示如何使用 dict 库实现记忆化递归，\n");
    printf("将时间复杂度从 O(2^n) 降低到 O(n)。\n\n");

    printf("时间复杂度对比:\n");
    printf("  普通递归:    O(2^n) - 指数级，每次都重新计算\n");
    printf("  记忆化:      O(n)   - 线性，每个值只计算一次\n\n");

    /* Verify correctness */
    if (verify_results() != 0) {
        printf("\n验证失败!\n");
        return 1;
    }

    /* Performance comparison */
    performance_comparison();

    /* Print cache statistics */
    print_cache_stats();

    /* Cleanup */
    if (g_fib_cache) {
        dict_destroy(g_fib_cache);
        g_fib_cache = NULL;
    }

    printf("========== 示例完成 ==========\n\n");

    return 0;
}
