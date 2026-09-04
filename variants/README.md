# 算法方向

仓库只保留每条开发方向当前最新的可运行版本，不保存全部历史迭代；历史由 Git commit
记录。根目录 `src.cpp` 是当前线上基线。

当前导入：

- `chatgpt-latest/src.cpp`：ChatGPT 网页资料库中下载时间最新的完整版本，原文件名
  `gomoku_v11_stable_clean.cpp`。

后续每个方向使用独立目录，例如：

- `time-control/src.cpp`：墙钟硬截止、初始化与内存优化；
- `tactical/src.cpp`：VCF/VCT、强制防守与禁手安全；
- `evaluation/src.cpp`：估值、候选点和着法排序。

任意两个方向都通过根目录的 `gomoku_match.py` 按相同规则交换黑白测试。
