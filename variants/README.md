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
- `rapfi-deadline-fusion/src.cpp`：原 Rapfi 棋力与硬墙钟融合。当前最强候选：绝对
  每手预算 + SIGALRM 看门狗兜底，2026-09-04 以 18:6 胜自身的旧时间管理版本。
  额外提供两个只在离线分析时生效的环境变量：`GOMOKU_DEADLINE_MS` 放宽搜索预算，
  `GOMOKU_EXCLUDE`（`行,列;行,列`）排除根着法以取次优解，供 `book_build.py` 造开局库。
- `rapfi-aggressive/src.cpp`：更高搜索预算边界实验。
- `rapfi-adaptive-time/src.cpp`：固定总预算下的自适应迭代控制。
- `rapfi-book-hybrid/src.cpp`：移植 figrid-board Standard 开局库。
- `replay-book-1726/src.cpp`：针对 UID 2025201726 某条开局主线的精确回放实验。
- `minimax-book-1726/src.cpp`：开局簿 + 小型 Negamax 的失败对照实验。

任意两个方向都通过根目录的 `gomoku_match.py` 按相同规则交换黑白测试。
