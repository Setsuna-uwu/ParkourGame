# 雪原跑酷 · 开工版

在 Windows 上双击根目录的 **run.bat**：它编译 C 玩法和 EasyX 窗口，然后启动程序。需要 x64 MinGW-w64（gcc 和 g++ 在 PATH 中）或 Visual Studio 的“使用 C++ 的桌面开发”。看到一条线和一个方块，按 Esc 退出，说明环境就绪。此时还没有滑行、跳跃、存档或计分。

## 三个人从哪里开始

先读 `src/main.c` 的循环：检查输入 → 更新角色 → 画一帧。想看函数内部，在编辑器里点函数名按 F12。源码中的 `TODO #7`、`TODO #8`、`TODO #9` 对应下面三张单。

| 任务 | 第一处改动 | 第一次怎样和队友接起来 |
|---|---|---|
| [#7 滑行和空翻](https://github.com/Setsuna-uwu/ParkourGame/issues/7) | `game.c` 保存角色的高度和速度；`draw.cpp` 读空格和画雪板。 | 先在平地做一次起跳，再拿 #8 提供的固定坡测试。把成功落地和摔倒两个一次性结果交给 #9。 |
| [#8 雪道与路线](https://github.com/Setsuna-uwu/ParkourGame/issues/8) | `terrain.c` 的 `ground_at` 先实现固定坡。用平地、缓坡和一段空隙验证坐标，再加随机生成。 | 早交一条可重复的测试坡，#7 可以立刻接；金币、生命的位置交给 #9 做拾取结算。 |
| [#9 一局与纪录](https://github.com/Setsuna-uwu/ParkourGame/issues/9) | `main.c` 循环里接一局的状态变化。内容增多时把规则放进自己的 `round.c`，在 run.bat 的两条 C 编译命令和两条链接命令中加入它。 | 先用临时输入模拟拾取、摔倒、落地和重开；再换成真实事件，最后试最高纪录跨进程保存。 |

## 三方公用的约定

- 距离和高度均按像素；世界距离从起点向右增加，屏幕 y 向下增加。角色脚底与地面比较，绘图也从同一处查地面。
- `ground_at(distance)` 返回 `Ground`。有雪面时使用其高度和坡度；遇到沟时 `has_ground` 为 0。先在当前接口上做，需要增改共享声明时在 Issue 通知另外两人。
- 三人各自从**合入本开工版后的 main** 开任务分支。先交固定坡与事件约定，再并行做完整功能；PR 写明操作、预期结果和实际试玩情况。详细 GitHub 操作见[协作指南](docs/team-workflow.md)。
- 旧平地跑酷、CMake 构建和原型测试可在[上一版](https://github.com/Setsuna-uwu/ParkourGame/tree/798a9b1eb14302a0ca5b42ed506c51d7fa7ff0c9)查阅。课设最终要展示的文件读写和制作者信息由 #9 实现。老师要求以根目录的课程设计原件为准。
