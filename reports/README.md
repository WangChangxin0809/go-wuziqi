# 比赛报告

- `round1.json`：首轮五候选循环赛。
- `semifinal.json`：基线、硬墙钟、Beam Search、Proof-Number 四强赛。
- `final.json`：基线与硬墙钟版四组开局、交换颜色的决赛。
- `online-wallclock-vs-2025201726.json`：与本地决赛并行的线上压力样本。
- `online-wallclock-vs-2025201726-clean.json`：硬墙钟版无并发线上复核。
- `online-baseline-vs-2025201726.json`：原版同条件线上对照。
- `online-replay-book-vs-2025201726.json`：精确主线回放，center-horizontal 双向复测 1:1。
- `online-rapfi-aggressive-vs-2025201726.json`、`online-rapfi-adaptive-time-vs-2025201726.json`：
  固定总预算的性能实验。
- `round2-fusion-clean.json`：修复 stdout 污染后的融合版本地对照。

JSON 保留每一步的选手、坐标、墙钟耗时、终局原因和远端状态，方便后续脚本重新统计，
不只依赖 README 中的结论。
