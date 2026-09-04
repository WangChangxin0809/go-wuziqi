# wallclock-fast

严格墙钟与启动性能方向。基于 `chatgpt-latest`，只改变计时基准：使用
`std::chrono::steady_clock`，并令搜索起点为进程启动时刻，因此 1 秒比赛预算
包含 C++ 运行时、棋盘重放和 AI 初始化，不会因 `clock()` 只统计 CPU 时间而超时。

编译：

```powershell
g++ -O2 -std=c++17 -Wall src.cpp -o wallclock-fast.exe
```

冒烟：

```powershell
py -3.11 ..\..\gomoku_match.py pair .\variants\wallclock-fast\wallclock-fast.exe .\variants\chatgpt-latest\src.cpp --games 2
```

风险：预算从进程启动计起，机器负载或冷启动较慢时可用于搜索的时间更少；这是
有意的严格边界版本。保持原有输入输出、恰好五连/长连及双三双四禁手适配。
