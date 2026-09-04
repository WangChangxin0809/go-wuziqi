# 算法方向

仓库只保留每条开发方向当前最新的可运行版本，不保存全部历史迭代；历史由 Git commit
记录。根目录 `src.cpp` 是当前线上基线。

当前导入：

- `chatgpt-latest/src.cpp`：ChatGPT 网页资料库中下载时间最新的完整版本，原文件名
  `gomoku_v11_stable_clean.cpp`。

当前并行候选：

- `wallclock-fast/src.cpp`：Rapfi 基线加全进程墙钟硬截止和快速初始化；
- `threat-solver/src.cpp`：轻量威胁搜索；
- `mcts-hybrid/src.cpp`：固定种子的 Monte-Carlo 根层抽样；
- `beam-search/src.cpp`：有限宽度的确定性多层搜索；
- `proof-number/src.cpp`：AND-OR/证明数式强制威胁求解。

任意两个方向都通过根目录的 `gomoku_match.py` 按相同规则交换黑白测试。
