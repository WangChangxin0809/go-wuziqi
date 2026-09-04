# BetaGomoku 网页接口

这些接口来自已登录页面中实际运行的前端 JavaScript，抓取时间为 2026-09-04。

## 接口

### `POST /api/exec`

请求头为 `Content-Type: application/json`，请求体：

```json
{"uid":"2025201726","input":"0\n-1 -1 ...\n"}
```

响应结构：

```json
{
  "output": "7 7",
  "result": {
    "status": 1,
    "time": 167702360,
    "memory": 35782656,
    "code": 0
  }
}
```

前端状态表：`1=OK`、`2=TLE`、`3=MLE`、`4=RE`、`5=Cancelled`、`6=OLE`。
`time` 很可能为纳秒、`memory` 很可能为字节，这是根据实测数量级推断，不是网站文档承诺。

### `POST /api/submit`

`multipart/form-data`，字段名为 `src`。响应：

```json
{"error":null,"compile":{"success":true,"diagnose":"..."}}
```

调用它会替换当前账号的线上提交，所以测试脚本不会自动调用该接口。

玩家 UID 没有单独接口；列表由 `GET /` 的 HTML `<option>` 服务端渲染。

## 保存登录态

登录 `gomoku-lab` Playwright 会话后：

```powershell
playwright-cli -s=gomoku-lab state-save .playwright\gomoku-auth.json
```

`.playwright/` 已被 `.gitignore` 排除，Cookie 不会上传 GitHub。

## 命令行调用

```powershell
py -3.11 .\gomoku_site_api.py players
py -3.11 .\gomoku_site_api.py exec 2025201726 .\position.txt
py -3.11 .\gomoku_site_api.py submit .\src.cpp
```

本地裁判的 `judge`、`judgeLongBan`、`judgeFourFourBan` 已按页面 JavaScript 逐句移植。
网页是在落子写入棋盘前执行禁手检查；Python 版本在写入后调用，但扫描从相邻格开始，
逻辑结果一致。特别注意：网页即使该点同时形成恰好五连，也仍会先判四四禁手。

标准对战器还支持直接把线上 UID 当作一名选手：

```powershell
py -3.11 .\gomoku_match.py pair .\variants\chatgpt-latest\src.cpp uid:2025201726 --games 2
```

线上选手由 `/api/exec` 运行，本地选手由本机每手新进程运行，胜负统一交给复刻的网页
JavaScript 裁判。这样可以在不覆盖自己线上提交的情况下，让本地候选和其他同学实战。
