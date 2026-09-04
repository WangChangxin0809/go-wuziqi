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
- `round3-timefix.json`：时间利用率修复前后，12 组开局交换黑白共 24 局。
- `round4-fastinit.json`：棋型表初始化 O(n^2) 改 O(n) 前后的 24 局对照，12:12。
- `round5-iteration.json`：迭代停止规则改预测式前后的 24 局对照，17:6:1。
- `round6-killer.json`：Killer moves 加成 24 的 24 局对照，8:16，已否。
- `round6-killer-bonus8.json`：同上但加成为 8，13:11，无收益，已否。
- `round7-book.json`：130 条开局库在 12 组开局下的 24 局回归，12:12，无退化。
- `round8-killer-sweep-{4,8,16}.json`：Killer moves 三个加成的 58 局对照，全部 50-52%，方向作废。
- `round9-forbidden-fix.json`：四四禁手判定对齐网页裁判后的 58 局回归，30:27:1。
- `round10-overline-fix.json`：长连不计五连修复后的 58 局回归，29:28:1。
- `round11-budget-half.json`、`round11-budget-third.json`：棋力对预算的敏感度，58% / 67%。
- `round12-forbid-prefilter.json`：禁手检查前置过滤的 58 局对照，31:26:1。
- `online-round12-1726-before-book.json`：对 1726 的首次线上实测，0:4。
- `online-round12-1726-deep15s.json`：我方 15 秒/手，执黑胜、执白负。
- `online-round12-1726-with-book.json`：加入对手特化开局库后 1 秒预算复测，1:1。
- `online-round12-1770.json`：1770 当前提交输出 `helloworld`，四局非法输出。

JSON 保留每一步的选手、坐标、墙钟耗时、终局原因和远端状态，方便后续脚本重新统计，
不只依赖 README 中的结论。
