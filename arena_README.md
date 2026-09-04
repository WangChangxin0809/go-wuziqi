# 本地 BetaGomoku 复刻评测

这是一个只依赖 Python 标准库的本地评测器，复刻作业网站的核心行为。胜负、长连和
四四判断已从 2026-09-04 的网页 JavaScript 原样移植：

- `g++ src.cpp -O2 -std=c++17 -Wall` 编译；
- 15×15 输入协议与 `0/1/-1` 棋盘；
- 单步 1 秒超时、运行错误、输出/落子合法性检查；
- 黑棋长连、四四禁手，双方恰好五连获胜；
- 逐手耗时、完整对局和黑白交换。

## 启动

```powershell
python .\arena_server.py
```

浏览器打开 <http://127.0.0.1:8765/>。网页里的 Submit 会把当前文件编译为
`players/local-current/engine.exe`。

## 比较多个库

为每个实现建立一个目录，目录名只用字母、数字、`.`、`_`、`-`，放入 `src.cpp`：

```text
players/
  rapfi2018/src.cpp
  rapfi2025/src.cpp
  pbrain_variant/src.cpp
```

重启服务或点击 Refresh engines；没有二进制时服务会自动编译。然后在两个下拉框
中交换选手跑对局。所有引擎都在本机执行，不影响线上提交次数。
