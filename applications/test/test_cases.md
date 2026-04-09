# 字典模块测试用例说明

## 1. 测试概述

| 项目 | 说明 |
|------|------|
| 测试框架 | Google Test (GTest) |
| 测试文件 | `test/dict_test.cpp`、`test/hash_test.cpp` |
| 测试用例总数 | 114 个 |
| 测试套件数 | 18 个 |
| 测试覆盖范围 | 配置/创建/销毁、CRUD操作、键类型支持（STRING/NUMBER/CHAR/SHORT/INT64/BINARY）、扩容/收缩、迭代器、哈希算法、边界条件 |

---

## 2. 测试套件清单

| 测试套件 | 用例数 | 说明 |
|----------|--------|------|
| `DictConfigTest` | 5 | 配置参数测试 |
| `DictCreateTest` | 3 | 创建和销毁测试 |
| `DictStringTest` | 27 | STRING 类型完整测试 |
| `DictNumberTest` | 5 | NUMBER int32/int64 类型测试 |
| `DictNumberShortTest` | 3 | NUMBER short 类型测试 |
| `DictNumberCharTest` | 3 | NUMBER char 类型测试 |
| `DictBinaryTest` | 3 | BINARY 结构体类型测试 |
| `DictResizeTest` | 2 | 自动扩容测试 |
| `DictCustomHashTest` | 2 | 自定义哈希函数测试 |
| `DictHashDistributionTest` | 2 | 哈希分布测试 |
| `DictNumberHashTest` | 2 | NUMBER 类型哈希测试 |
| `DictBoundaryTest` | 6 | 边界条件测试 |
| `DictStressTest` | 7 | 压力测试 |
| `DictShrinkTest` | 12 | 收缩测试 |
| `DictExistsTest` | 7 | 存在性查询测试 |
| `DictCapacityTest` | 7 | 容量测试 |
| `DictIteratorTest` | 10 | 迭代器测试 |
| `MurmurHashTest` | 7 | MurmurHash3 算法测试 |

---

## 3. 关键测试用例

### 3.1 配置测试 (DictConfigTest) - 5个

| 测试用例 | 说明 |
|---------|------|
| `DefaultConfig` | NULL 配置使用默认参数（容量32、负载因子0.75） |
| `CustomConfig` | 自定义容量、负载因子 |
| `InvalidLoadFactor` | 无效负载因子（0 或 >1）被修正 |
| `MinCapacity` | 容量向下对齐到最小值 |
| `MaxCapacity` | 容量向上对齐到 2^n |

### 3.2 创建和销毁测试 (DictCreateTest) - 3个

| 测试用例 | 说明 |
|---------|------|
| `CreateAndDestroy` | 正常创建和销毁 |
| `DestroyNull` | 销毁 NULL 句柄返回 DICT_EINVALID |
| `DoubleDestroy` | 重复销毁返回错误 |

### 3.3 STRING 类型测试 (DictStringTest) - 27个

| 测试用例 | 说明 |
|---------|------|
| `SetAndGet` | 基础插入和查询 |
| `SetAndGetMultiple` | 插入多个不同键值对 |
| `Overwrite` | 覆盖已存在的键 |
| `Delete` | 删除已存在的键 |
| `DeleteNotFound` | 删除不存在的键返回 ENOTFOUND |
| `DeleteNull` | NULL 键参数返回错误 |
| `ClearNull` | 清空 NULL 句柄返回 DICT_EINVALID |
| `ClearEmpty` | 清空空字典成功 |
| `ClearWithData` | 清空有数据字典 |
| `NullValue` | 支持空值（len=0） |
| `OverwriteNullWithValue` | 空值覆盖为实际值 |
| `EmptyKey` | 空字符串键 |
| `SpecialCharsInKey` | 特殊字符键 |
| `LongKey` | 255字节长键 |
| `DuplicateKey` | 重复键覆盖 |
| `SizeAndCount` | size() 和 get_size() 正确 |
| `GetAfterDelete` | 删除后查询返回 ENOTFOUND |
| `Exists` | exists() 正确判断存在性 |
| `NotExists` | 不存在的键返回 false |
| `ExistsAfterClear` | 清空后 exists() 返回 false |
| `BufferTooSmall` | 缓冲区不足返回 ETOOSMALL |
| `NullParams` | NULL 参数处理 |
| `GetSize` | get_size() 获取值大小 |
| `GetSizeNotFound` | 不存在键获取大小失败 |
| `GetSizeNullParam` | NULL 参数返回错误 |
| `IterateEmpty` | 空字典迭代 |
| `IterateWithData` | 有数据字典迭代 |

### 3.4 NUMBER 类型测试 (DictNumberTest) - 5个

| 测试用例 | 说明 |
|---------|------|
| `Int32` | int32_t 类型键基础 CRUD |
| `Int64` | int64_t 类型键基础 CRUD |
| `UpdateValue` | 更新已有键的值 |
| `AllTypes` | 所有 NUMBER 子类型共存 |

### 3.4.1 NUMBER char 类型测试 (DictNumberCharTest) - 3个

| 测试用例 | 说明 |
|---------|------|
| `CharKey` | char 类型键基础 CRUD |
| `MultipleCharKeys` | 插入所有可能的 char 值（-128 到 127），共 256 个键值对，验证存在性 |
| `UpdateAndDelete` | char 键的更新和删除操作 |

### 3.4.2 NUMBER short 类型测试 (DictNumberShortTest) - 3个

| 测试用例 | 说明 |
|---------|------|
| `ShortKey` | short 类型键基础 CRUD |
| `MultipleShortKeys` | 插入 50 个 short 键，验证数据完整性、删除操作 |
| `BoundaryValues` | 测试 short 边界值（0, -1, 1, SHRT_MAX, SHRT_MIN） |

### 3.5 BINARY 结构体类型测试 (DictBinaryTest) - 3个

| 测试用例 | 说明 |
|---------|------|
| `BinaryData` | 存储结构体数据 |
| `OverwriteBinary` | 覆盖结构体 |
| `GetSize` | 获取结构体大小 |

### 3.6 扩容测试 (DictResizeTest) - 2个

| 测试用例 | 说明 |
|---------|------|
| `AutoResize` | 容量从 4 自动扩容到 32 |
| `NumberTypeResize` | NUMBER 类型键的扩容 |

### 3.7 自定义哈希测试 (DictCustomHashTest) - 2个

| 测试用例 | 说明 |
|---------|------|
| `HashCollisionViaCustomHash` | 自定义哈希函数产生碰撞 |
| `DefaultHash` | 默认 MurmurHash3 |

### 3.8 哈希分布测试 (DictHashDistributionTest) - 2个

| 测试用例 | 说明 |
|---------|------|
| `Distribution` | 100 个键值对的分布均匀性 |
| `LoadFactor` | 负载因子计算 |

### 3.10 NUMBER 哈希测试 (DictNumberHashTest) - 2个

| 测试用例 | 说明 |
|---------|------|
| `DifferentKeySizes` | 测试不同长度键哈希（uint8_t/uint16_t/uint32_t/uint64_t） |
| `HashConsistency` | 哈希一致性验证 |

### 3.11 边界条件测试 (DictBoundaryTest) - 6个

| 测试用例 | 说明 |
|---------|------|
| `ZeroCapacity` | 零容量使用默认值 |
| `VeryLargeValue` | 大值（>1024字节） |
| `MaxKeySize` | 最大键大小 |
| `EmptyDict` | 空字典操作 |
| `FullDict` | 字典填满到阈值 |
| `NullHandle` | NULL 句柄处理 |

### 3.11 压力测试 (DictStressTest) - 7个

| 测试用例 | 说明 |
|---------|------|
| `ManyEntries` | 1000 个键值对 |
| `RepeatedClear` | 重复插入和清空 10 轮 |
| `ContinuousResize` | 频繁扩容（容量2起步） |
| `UpdateHeavy` | 高频更新同一键 10 次 |
| `DeleteHalf` | 删除一半数据 |
| `FillAndEmpty` | 填满后清空循环 |
| `RandomOrder` | 随机顺序操作 |

### 3.12 收缩测试 (DictShrinkTest) - 12个

| 测试用例 | 说明 |
|---------|------|
| `NullHandle` | NULL 句柄返回错误 |
| `EmptyDictShrink` | 空字典收缩 |
| `AfterDelete` | 删除后收缩 |
| `AfterClear` | 清空后收缩 |
| `StringTypeShrink` | STRING 类型收缩 |
| `NumberTypeShrink` | NUMBER 类型收缩 |
| `BinaryTypeShrink` | BINARY 类型收缩 |
| `HalfOccupied` | 一半占用时收缩 |
| `MostOccupied` | 高占用时收缩 |
| `CustomCapacity` | 自定义容量收缩 |
| `AfterUpdate` | 更新后收缩 |
| `AfterShrinkAgain` | 再次收缩 |

### 3.13 存在性测试 (DictExistsTest) - 7个

| 测试用例 | 说明 |
|---------|------|
| `Exists` | 存在的键返回 true |
| `NotExists` | 不存在的键返回 false |
| `AfterDelete` | 删除后返回 false |
| `AfterClear` | 清空后返回 false |
| `NumberTypeKey` | NUMBER 类型键存在性 |
| `BinaryTypeKey` | BINARY 类型键存在性 |
| `MultipleKeys` | 多个键的存在性 |

### 3.14 容量测试 (DictCapacityTest) - 7个

| 测试用例 | 说明 |
|---------|------|
| `InitialCapacity` | 初始容量 |
| `AfterResize` | 扩容后容量 |
| `AfterShrink` | 收缩后容量 |
| `PowerOfTwo` | 2^n 对齐验证 |
| `NonPowerOfTwo` | 非 2^n 对齐 |
| `MaxCapacity` | 最大容量限制 |
| `ResizeGrowth` | 容量增长倍数 |

### 3.15 迭代器测试 (DictIteratorTest) - 10个

| 测试用例 | 说明 |
|---------|------|
| `NullParams` | NULL 参数处理 |
| `ConditionDelete` | 条件删除迭代 |
| `IterateAll` | 遍历所有元素 |
| `IterateAfterDelete` | 删除后继续迭代 |
| `IterateAfterClear` | 清空后迭代结束 |
| `IterateWhileInsert` | 迭代时插入 |
| `IterateWhileDelete` | 迭代时删除 |
| `EmptyDict` | 空字典迭代 |
| `SingleElement` | 单元素迭代 |
| `ModifyDuringIterate` | 迭代中修改 |

### 3.16 哈希算法测试 (MurmurHashTest) - 7个

| 测试用例 | 说明 |
|---------|------|
| `VerifyInternalVectors` | 26 个预计算向量验证 |
| `EmptyString` | 空字符串哈希 0x373AA46D |
| `SingleChar` | 单字符哈希 0xE2D9D15B |
| `PartialLength` | 部分长度哈希 |
| `DifferentSeeds` | 不同种子哈希 |
| `CollisionResistance` | 碰撞抵抗性 |
| `SameInputSameOutput` | 幂等性验证 |

---

## 4. 测试覆盖矩阵

| 功能区域 | 用例数 | 覆盖率 |
|---------|-------|-------|
| 配置 | 5 | 100% |
| 创建/销毁 | 3 | 100% |
| STRING 类型 | 27 | 100% |
| NUMBER int32/int64 类型 | 5 | 100% |
| NUMBER char 类型 | 3 | 100% |
| NUMBER short 类型 | 3 | 100% |
| BINARY 类型 | 3 | 100% |
| 插入(set) | 全部 | 100% |
| 查询(get) | 全部 | 100% |
| 删除(delete) | 全部 | 100% |
| 清空(clear) | 3 | 100% |
| 扩容 | 2 | 100% |
| 收缩 | 12 | 100% |
| 迭代器 | 10 | 100% |
| 哈希算法 | 7 | 100% |
| 边界条件 | 6 | 100% |
| 压力测试 | 7 | 100% |

---

## 5. 运行测试

```bash
# 编译
scons

# 运行所有测试
./build/bin/dict_test

# 运行特定测试套件
./build/bin/dict_test --gtest_filter=DictStringTest.*

# 运行特定测试用例
./build/bin/dict_test --gtest_filter=DictStringTest.SetAndGet

# 查看测试列表
./build/bin/dict_test --gtest_list_tests

# 生成 XML 报告
./build/bin/dict_test --gtest_output=xml:report.xml
```
