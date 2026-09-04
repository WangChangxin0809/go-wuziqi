# rapfi-adaptive-time

基于 `rapfi-deadline-fusion` 的固定总墙钟实验：每步统一从进程启动计时至
840ms 截止。内部按局面自适应：开局库或确定性即时回应直接返回；强制胜/防路径
优先，安静局把剩余时间交给 Alpha-Beta 迭代加深，并依据上一层耗时和剩余预算决定
是否继续。保留约160ms协议尾部安全、steady_clock、网页精确禁手、合法 fallback
与 stdout 诊断隔离。

```powershell
g++ -O2 -std=c++17 -Wall src.cpp -o rapfi-adaptive-time.exe
```
