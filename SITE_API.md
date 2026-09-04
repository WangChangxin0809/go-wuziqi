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

整站在 RUC 统一身份 OAuth2 之后：`GET /` 会 302 到 `/users/login`，唯一入口是
「微人大认证」，命令行无法自行完成登录。所以必须先在浏览器里登录一次，再把会话
Cookie 交给脚本。`gomoku_site_api.py` 按下面的顺序取凭据，用上第一个存在的：

1. `--cookie 'connect.sid=...'`
2. 环境变量 `GOMOKU_COOKIE`
3. `.playwright/gomoku-cookie.txt`
4. `.playwright/gomoku-auth.json`（Playwright storage state）

方式 A —— 手工粘贴 Cookie（浏览器登录后，DevTools → Network → 任意请求 →
Request Headers → 复制 `Cookie:` 整行）：

```powershell
mkdir .playwright -Force
'connect.sid=s%3A...' | Out-File -Encoding utf8 .playwright\gomoku-cookie.txt
```

文件里可以直接留 `Cookie: a=1; b=2` 这一整行，也可以只写 `name=value`；`#` 开头的
行会被忽略。

方式 B —— Playwright 会话，登录 `gomoku-lab` 之后：

```powershell
playwright-cli -s=gomoku-lab state-save .playwright\gomoku-auth.json
```

`.playwright/` 已被 `.gitignore` 排除，Cookie 不会上传 GitHub。

## 命令行调用

```powershell
py -3.11 .\gomoku_site_api.py check
py -3.11 .\gomoku_site_api.py players
py -3.11 .\gomoku_site_api.py exec 2025201726 .\position.txt
py -3.11 .\gomoku_site_api.py submit .\src.cpp
```

`check` 只做一次最轻的已登录探测。Cookie 失效时，脚本不会返回登录页 HTML，而是
报错退出（exit code 2），提示重新取 Cookie。

本地裁判的 `judge`、`judgeLongBan`、`judgeFourFourBan` 已按页面 JavaScript 逐句移植。
网页是在落子写入棋盘前执行禁手检查；Python 版本在写入后调用，但扫描从相邻格开始，
逻辑结果一致。特别注意：网页即使该点同时形成恰好五连，也仍会先判四四禁手。

标准对战器还支持直接把线上 UID 当作一名选手：

```powershell
py -3.11 .\gomoku_match.py pair .\variants\chatgpt-latest\src.cpp uid:2025201726 --games 2
```

线上选手由 `/api/exec` 运行，本地选手由本机每手新进程运行，胜负统一交给复刻的网页
JavaScript 裁判。这样可以在不覆盖自己线上提交的情况下，让本地候选和其他同学实战。
