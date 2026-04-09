# -*- coding: utf-8 -*-
"""
SConstruct for dict project
"""

import os

# 项目根目录
root_dir = Dir('.')

# 设置编译环境
env = Environment(
    CPPPATH = [
        root_dir.Dir('dict/inc'),
    ],
)

# AddressSanitizer 支持（内存泄漏检测）
# 默认启用，使用 scons USE_ASAN=0 可禁用
use_asan = ARGUMENTS.get('USE_ASAN', '1')
if use_asan == '1':
    asan_flags = ['-fsanitize=address', '-fno-omit-frame-pointer']
    env.Append(CCFLAGS = asan_flags)
    env.Append(CXXFLAGS = asan_flags)
    env.Append(LINKFLAGS = ['-fsanitize=address'])
    print("[ASAN] AddressSanitizer enabled - memory leak detection active")
else:
    print("[ASAN] AddressSanitizer disabled")

# PC 模式宏定义
env.Append(CCFLAGS = ['-DDICT_RUN_ON_PC'])
env.Append(CXXFLAGS = ['-DDICT_RUN_ON_PC'])

# C 文件编译选项
env.Append(CCFLAGS = ['-Wall', '-Wextra', '-g', '-O0'])
env.Append(CFLAGS = ['-std=c99'])

# C++ 文件编译选项 - Google Test 需要 C++14
env.Append(CXXFLAGS = ['-std=c++14', '-Wall', '-Wextra', '-g', '-O0'])

# 导出环境变量供子模块使用
Export('env')

# 打印构建信息
print("=" * 50)
print("Building dict library and tests...")
print("=" * 50)

# 先构建核心库
lib, lib_path = SConscript('dict/SConscript')

# 导出库对象供其他模块使用
Export('lib', 'lib_path')

# 再构建测试（依赖库）
test_prog = SConscript('test/SConscript')

# 添加测试目标
Alias('test', test_prog)

# 构建示例程序
example_prog = SConscript('examples/SConscript')
Alias('examples', example_prog)

# 设置默认目标（包含测试和示例）
Default([test_prog, example_prog])

# 清理目标
Clean('.', 'build')
