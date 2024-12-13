# PBA 加速算法

本算法旨在加速PBA计算。以下是对OT改动和加速方案的具体实现：

---

## 主要改动与加速方案

### 1. **在 `arc.cpp` 中实现：**

- **`_get_pba_timing()`**：
  用于对单个时序弧进行PBA下的时序计算。
  
  由于OT本身的特性，时序计算只能将 **net** 与 **cell** 的时序计算分开。目前实现了以下方案：
  - **Net**：使用SPICE仿真和PRIMA降阶算法进行时序计算。
  - **Cell**：时序计算依然采用Lumped Cap查表方式。

---

### 2. **在 `path.cpp` 中实现：**

- **`report_timing_pba()`**：
  用于对一组路径（paths）进行完整的PBA时序计算。

- **`report_timing_pba_merge()`**：
  本算法的核心实现函数。具体过程如下：
  
  1. 将整条路径分割成多个 `path_segment`，每个**path_segment**的起始点是时序图中的合并点（即路径上具有多个fan-in的pin），或者路径的第一个pin。
  2. 当遇到**path_segment**的起始点时，检查该次计算过程中当前pin上的输入slew是否已计算过。具体检查方法是查看该**path_segment**是否存在于哈希表中。
  3. 如果计算过：
     - 从哈希表中获取该**path_segment**的时序信息，并将其复制到当前被计算的**path_segment**中。
  4. 如果没有计算过：
     - 调用 `_get_pba_timing()` 重新计算该**path_segment**中所有弧（arc）的时序信息。

---

### 3. **哈希表的实现：**

使用哈希表 `unordered_map<size_t, map<float, pair<Path *, size_t>> path_segments` 存储计算过的path_segment的时序信息。

- 哈希表存储的是路径的地址 `Path *` 以及该**path_segment**在路径中的起始点索引 `size_t`。
- 计算时，首先通过**path_segment**的起始点的**arc**`size_t`作为键查找哈希表。
- 如果存在对应条目，则返回一个 `map`，其 `key` 为 **slew**`float`，只有在 **slew** 在允许的误差范围内时，才会发生时序信息的复制。复制的信息**value**包括路径地址和起始点的索引 `pair<Path *, size_t>`。

---

## 如何运行

本项目在每个case下会生成一个 `case.exe` 可执行文件。通过运行该可执行文件，可以依次进行以下步骤：

1. **GBA及路径搜索**
2. **Full PBA**
3. **Merge PBA**
4. **统计并输出信息**

### 输入参数：

- **case_name**：指定Case的名称。
- **path_num**：指定要搜索的路径数目。
- **acceptable_slew**：可接受的slew误差范围（必须大于0.0，否则不会执行Merge PBA）。

### 编译与运行：

你可以使用 `cmp_run.sh` 脚本来进行编译和运行。
