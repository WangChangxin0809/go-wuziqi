# 开局库

`book_build.py` 用放宽到秒级的搜索预算离线跑同一个引擎，把它下出来的棋记下来。
比赛时 1 秒查表命中，等于白拿深度：评测每手重启进程、单独计时，运行时买不到这个深度。

## 为什么值得做

先用 `probe` 验证过两件事，再决定投入：

- 长考确实会改变着法：14 手自对弈主线上分歧率 21%；黑方节点上 32 个里 13 个是修正（41%）。
- 这些分歧不是噪声：同一局面用比赛预算跑 4 次，着法完全一致，搜索是确定的。

## 文件

- `book-black-v2.json`：黑方，12 手深、对手每步展开 3 条回应，210 节点存 44 条。
- `book-white.json`：白方，同参数，412 节点存 86 条。
- `book-black.json`：第一版黑方库（branch 2），13 条，保留作对照。

合计 130 条已由 `emit` 编入 `src.cpp`。

## 重新生成

```sh
python3 book_build.py probe src.cpp --plies 14
python3 book_build.py build src.cpp --side black --plies 12 --branch 3 \
  --long-ms 15000 --reply-ms 4000 --only-corrections --jobs 3 --out books/book-black-v2.json
python3 book_build.py build src.cpp --side white --plies 12 --branch 3 \
  --long-ms 15000 --reply-ms 4000 --only-corrections --jobs 3 --out books/book-white.json
python3 book_build.py emit src.cpp books/book-black-v2.json books/book-white.json
```

`emit` 把表写进源码里 `/* BOOK-BEGIN */` 和 `/* BOOK-END */` 之间，不要手工编辑那一段。
`--jobs` 应低于核心数：每个 worker 跑自己的引擎进程，抢 CPU 会直接缩短每次搜索的实际深度。

## 表示方式

局面按棋盘的 8 个对称取字典序最小的那个朝向作为规范形，键是规范形 225 字符平面的
FNV-1a 64 哈希，着法按 `行 * 15 + 列` 存在同一朝向下。所以一条记录覆盖它的全部 8 个
旋转和镜像；查表时把命中的着法按逆变换转回来。Python 和 C++ 两侧的变换顺序与哈希
必须逐位一致，`book_build.py` 的 `TRANSFORMS` 对应源码的 `contestTransform`，
`fnv1a64` 对应 `contestBookHash`。

## 当前效果

空盘双向、对三个共同对手：有书 6-0，无书 5-1；12 组强制开局 24 局回归 12-12。
证据偏弱但方向为正、无退化，运行时代价是一次亚毫秒的二分查找。

强制开局大多落在书外，所以评测必须用空盘——比赛本来就是从空盘开始的。
