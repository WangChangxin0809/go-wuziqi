# replay-book-1726

本方向以 rapfi-deadline-fusion 为 fallback，并加入针对 uid:2025201726 的 replay book。书着来源于 reports/online-baseline-vs-2025201726.json 中 center-horizontal 对局的完整逐手记录。

每个书着只在当前 15x15 棋盘与记录完全一致时启用；一旦对手任意一步偏离，立即转入 fallback。刻意不使用模糊 subset 匹配，因为子集匹配可能在不同历史局面误触发，造成非法或战术错误；书着启用前仍执行根层即时胜/必防和网页禁手过滤。

当前线上复测：center-horizontal 双向各 1 局，结果 1:1；其中候选执黑在第 43 手以 Five 获胜，书内着法平均约 85ms。该书仅针对该对手、该开局方向和已记录线路有效，不能视为通用棋力提升。
