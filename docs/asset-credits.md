# 素材来源与许可

## 1. 总览

本项目 MVP **没有使用任何外部图片、精灵、音频或字体文件**。

| 类别 | 来源 | 许可/说明 |
|---|---|---|
| EasyX 图形库 | EasyX 官方 26.10.25 | 免费使用；版权归 EasyX 作者 |
| 游戏画面 | 项目内 EasyX 几何图元程序化绘制 | 本项目产物 |
| 字体 | Windows 系统自带 `Microsoft YaHei UI` | 随 Windows 使用 |
| 音频 | 无 | MVP 明确不做 |

## 2. EasyX

- 名称：EasyX Graphics Library
- 版本：26.10.25（2026-10-25）
- 官网：<https://easyx.cn/>
- 下载页：<https://easyx.cn/t/download>
- 版权声明：<https://easyx.cn/copyright>
- 官方声明摘要：“EasyX 是免费的，任何人均可使用。”
- 本项目使用官方 Zip 版本，解出：

```text
third_party/easyx/include/easyx.h
third_party/easyx/include/graphics.h
third_party/easyx/lib/x64/EasyXw.lib
third_party/easyx/lib/x64/EasyXa.lib
third_party/easyx/lib/mingw/x64/libeasyxw.a
third_party/easyx/lib/mingw/x64/libeasyx.a
```

未修改 EasyX 头文件与库文件。

### 文件哈希（SHA-256）

```text
27B06C29B7115EDBC30196FAAA1D6D83EF90EEE77D90D25E30443F3A3A344A9F  include/easyx.h
E53B8FC22D6C377DC38DE809CE3B066B7FD3C3A5ACB64141280352B52E87864D  include/graphics.h
597FB3D54915E972CF71F01704C4E59090CBA750848774B9C390FD4E35DE76DA  lib/x64/EasyXw.lib
8EC8A192D6825D7BE151E5E8758028CF384365CBE978C17EFAE8C8EFA9DC8FF0  lib/x64/EasyXa.lib
2BADBF5481392A8E7249734A0990F3BCC1708AF5FC0418148C00F4E97C042A0D  lib/mingw/x64/libeasyxw.a
92B2A1A413D74ACCE7033C923133564314F64315FCBB7FF4EF72850E5F56A8B5  lib/mingw/x64/libeasyx.a
```

## 3. 程序化美术

所有场景元素均由 `src/draw.cpp` 使用 EasyX 图元绘制：

- 夜空渐变、星、月、三层视差雪山、松林；
- 雪地、土层、石块、障碍；
- 由圆角矩形/圆/线段组成的角色；
- HUD、暂停、结算、调试面板。

没有从网络抓取图片，也没有使用游戏素材包。


## 4. 字体

渲染调用：

```c
settextstyle(..., L"Microsoft YaHei UI", ...);
```

该字体是 Windows 10/11 自带字体，不随项目分发字体文件。若目标系统缺少该字体，EasyX/Windows 会回退到系统默认 UI 字体。

## 5. 第三方工具（仅构建期）

- CMake
- Visual Studio / MSVC
- Windows SDK

这些工具不进入最终游戏资源目录。
