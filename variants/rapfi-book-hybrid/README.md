# rapfi-book-hybrid

基于 `rapfi-deadline-fusion` 的搜索 fallback，加入 `nicotina04/figrid-board`
v0.8.6 `src/book.rs` 的 12 条 `standard` 开局及 Rapfi 30s 回复。表中坐标与
来源说明保留在 `book.rs`；该文件原许可为 MIT OR Apache-2.0。运行时对当前棋盘
做 8 对称匹配，命中点仍检查空位和网页黑方长连/四四规则，未命中则进入融合版搜索。

预留后续添加 1726 主线表的位置：继续向 `STANDARD_BOOK` 追加条目即可。

```powershell
g++ -O2 -std=c++17 -Wall src.cpp -o rapfi-book-hybrid.exe
```
