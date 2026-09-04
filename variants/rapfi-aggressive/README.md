# rapfi-aggressive

性能边界实验版，基于 `rapfi-deadline-fusion`。全进程 `steady_clock` 截止为
820ms，搜索直接使用剩余墙钟（不做整场 MATCH_SPARE 分摊），为启动和输出保留约
180ms 安全空间；保留网页精确禁手、合法 fallback 与 stdout 诊断隔离。

```powershell
g++ -O2 -std=c++17 -Wall src.cpp -o rapfi-aggressive.exe
py -3.11 ..\..\gomoku_match.py probe src.cpp ..\..\fixtures\white-win-11-6.txt
```
