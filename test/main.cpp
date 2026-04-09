/**
 * @file main.cpp
 * @brief 单元测试主函数入口
 */

#include <gtest/gtest.h>

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define ASAN_ENABLED 1
#endif
#endif

#if defined(__SANITIZE_ADDRESS__)
#define ASAN_ENABLED 1
#endif

int main(int argc, char **argv)
{
    printf("==================================================\n");
    printf("  Dictionary and Hash Unit Tests\n");
    printf("==================================================\n\n");

    ::testing::InitGoogleTest(&argc, argv);

    int result = RUN_ALL_TESTS();

#if defined(ASAN_ENABLED)
    printf("\n[ASAN] Memory leak check: The test runner exits.\n");
    printf("[ASAN] If leaks are detected, ASAN will print leak report above.\n");
#endif

    return result;
}
