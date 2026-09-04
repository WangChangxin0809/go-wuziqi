# Rapfi 2018 单文件移植版

`src.cpp` 可直接按题目命令提交：

```sh
g++ src.cpp -O2 -o bin -std=c++17 -Wall
```

程序读取执子颜色和 15×15 局面，只向标准输出写一行 `行 列`。

## 上游与取舍

当前 `src.cpp` 是 [Rapfi 2018 官方源码](https://github.com/dhbloo/Rapfi-gomocup)
的完整单文件合并版，保留原版 3876 项内置棋型权重、增量棋型更新、迭代加深、
Alpha-Beta/PVS、置换表、威胁生成和 VCF 搜索，不是重新仿写的简化搜索器。

在原版上只做了三类必要改动：Linux/GCC 可移植性、题目的矩阵输入输出、题目特有规则。
规则适配为黑棋只禁长连和四四、不禁三三，双方都只有“恰好五连”获胜。四型按实际四子
集合去重，避免把一个有两个胜点的活四误判成四四。

最新 Rapfi 2025 是 200 多文件工程，并依赖外置 Mix9SVQ/NNUE 权重，无法原封不动塞入
只提交一个源码文件、运行限制 1 秒的评测接口。2018 版采用 MIT 许可证，完整许可证已保留
在 `src.cpp` 文件头。

## 验证

- 使用题目原样命令编译，零警告。
- 验证立即取胜、强制防守、长连禁手、四四禁手、白棋双四合法和棋盘边缘。
- 6 组随机中盘全部返回合法空点。
- 与旧的简化版完整对弈两盘，新版分别执黑、执白均获胜。

## 本地复刻评测

`arena_server.py` + `arena.html` 提供不依赖第三方库的本地 BetaGomoku 评测器，
可将多个实现放入 `players/<name>/src.cpp` 后进行编译、逐手计时和双向对局。
详见 [arena_README.md](arena_README.md)。

命令行标准对战器为 `gomoku_match.py`。它每一手都会重新启动选手程序，传入静态棋盘，
使用 1 秒墙钟限制，并检查输出、重复落子、黑棋长连/四四禁手和恰好五连。两个版本
会交换黑白：

```powershell
python .\gomoku_match.py pair .\src.cpp .\variants\chatgpt-latest\src.cpp `
  --games 2 --json .\reports\baseline-vs-chatgpt-latest.json
```

多个方向可运行循环赛：

```powershell
python .\gomoku_match.py matrix .\src.cpp .\variants\*\src.cpp --games 2
```

登录态保存后，也可以把评测网站上的 UID 当作选手，与本地版本交换黑白：

```powershell
py -3.11 .\gomoku_match.py pair .\src.cpp uid:2025201726 --games 2
```

固定局面回归（可重复使用 `--expect` 接受多个正确应手）：

```powershell
py -3.11 .\gomoku_match.py probe .\src.cpp .\fixtures\empty-black.txt --expect 7,7
```

仓库只保留每条方向最新版本，旧版本由 Git 历史保存，目录约定见
[variants/README.md](variants/README.md)。

已登录网页的 `/api/exec`、`/api/submit` 调用方法和本地客户端见
[SITE_API.md](SITE_API.md)。认证 Cookie 只保存在被忽略的 `.playwright/` 目录中。
候选淘汰、决赛与线上挑战规则见 [TOURNAMENT.md](TOURNAMENT.md)。
