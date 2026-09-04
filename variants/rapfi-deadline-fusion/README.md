# rapfi-deadline-fusion

根目录 `src.cpp` 的棋力基线，加上从进程启动计时的 `steady_clock` 硬截止。
启动、输入解析和棋盘重放都会计入 760ms 总预算；搜索通过动态 `timeLeft()` 自动收缩。

```powershell
g++ -O2 -std=c++17 -Wall src.cpp -o rapfi-deadline-fusion.exe
```
