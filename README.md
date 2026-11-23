# 雪原跑酷

Windows x64 上双击 `scripts/run.bat`，或在任意编辑器的终端运行 `.\scripts\run.bat`。脚本直接调用编译器，只需要以下任意一套，EasyX 已随项目附带：

- **MinGW-w64 x64**：把包含 `gcc.exe` 和 `g++.exe` 的目录加入 PATH，两条命令都要能用。
- **Visual Studio 或 Build Tools**：在安装器里选“使用 C++ 的桌面开发”。脚本找不到 MinGW-w64 时会尝试加载已安装的 x64 MSVC。

游戏生成在 `build/local/Release/SnowParkour.exe`。只试玩可以打开已编译的 Release 程序。改了源码再双击脚本重新编译。已有 CMake 工作流也可以继续使用 `CMakeLists.txt`，平时双击运行和测试都无需安装 CMake 或 Ninja。

空格跳跃，空中再按一次可以二段跳。P 暂停或继续，撞上障碍后按 R 或点鼠标重开，Esc 退出。F1 显示碰撞框。最高分保存在游戏程序旁边的 highscore.dat。

## 从一次跳跃读起

1. 打开 [main.c](src/main.c)。找到 while 循环，读键、调用 game_update、画画面，每轮走一步。
2. 打开 [game.c](src/game.c)，从 game_update 找到 move_player。keys.jump_pressed 表示刚刚按下空格。player.vy 是竖直速度，屏幕坐标向下增大，所以起跳时速度为负数。函数算完把新的角色交还给 game.player。
3. 想改跳跃高度，在 [game.h](src/game.h) 找 SP_JUMP_SPEED。这个文件还定义了 Player 等结构体。player.y 就是角色的纵坐标。
4. 想改障碍排列，看 [terrain.c](src/terrain.c) 开头的 PATTERNS。每行写了一组障碍的类型和间距，地面目前是一条水平线。

[draw.cpp](src/draw.cpp) 用 EasyX 画出当前游戏状态。[windows.cpp](src/windows.cpp) 处理窗口、按键、计时和存档。做跳跃和计分时主要看 C 文件即可。

## 小组协作与验证

按[三人协作指南](docs/team-workflow.md)接任务、分支提交和验收。先试玩一次跳跃，再故意撞上障碍重开。需要回归检查时运行 scripts/test.bat Debug。测试集中在 tests/test.c，平时启动游戏只编译游戏本身。

后续计划是随机下坡滑雪、断崖和额外生命，当前分支保留平地跑酷原型。
