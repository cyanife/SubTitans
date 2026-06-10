# SubTitans · 深海争霸汉化兼容修改版

本仓库基于 UnknownException 的 [SubTitans](https://github.com/UnknownException/SubTitans)（《深海争霸》非官方补丁）修改，为[深海争霸简体中文补丁](https://github.com/cyanife/Submarine-Titans-Chinese)提供加载与兼容支持。上游全部功能均已保留。

## 安装说明

1. 前往本仓库的 **[Releases](https://github.com/cyanife/SubTitans/releases/latest)** 页面，下载 `SubTitans-CN.zip`

2. 拷贝压缩包内的d3drm.dll、subtitans.dll和subtitans.ini到已经安装好汉化补丁的深海争霸安装目录下。

## 与上游的差异

### 一、汉化（HAIGU）加载与集成

`HAIGU.dll` 是 2001 年奥美娱乐官方汉化的 MFC 注入式 DLL，通过 4 处 inline hook 接管游戏引擎的文字排版与渲染流程，实现中文显示。本修改版使其在现代系统上与 SubTitans 共存：

- **d3drm 注入器**：在 v1.1 代码布局下自动加载 HAIGU，并确保其先于 SubTitans 初始化。
- **渲染器协作**：补全 DirectDraw 设备指针与调色板接口，并为 HAIGU 字幕行提供逐帧调色板同步。
- **混装保护**：通过导出标记检测原版 subtitans.dll，防止 hook 冲突导致崩溃。

### 二、兼容性修复

- **HAIGU 数字渲染修复**：在 HAIGU 字形接管入口添加 ASCII 绕过，修复现代 Windows 上数字叠加成色块的问题。
- **界面文案汉化**：汉化加载时，设置菜单中替代 1024x768 的 "NATIVE RESOLUTION" 选项显示为"原生分辨率"。
- **`Surface::ReleaseDeviceContext` 回拷修复**（`subtitans/surface.cpp`）：上游缺少 GDI 内存 DC 缓冲到表面缓冲的回拷，GDI 绘制结果会丢失。

### 三、行为说明

- 游戏目录中无 `HAIGU.dll`（未安装汉化）时，本修改版行为与上游完全一致。
- 删除 `subtitans.dll` 可关闭高分辨率渲染等增强功能，仅保留 d3drm + 汉化的最小配置，数字渲染修复仍然有效。`d3drm.dll` 不可删除。

---

# 以下为原版说明（英文）

# SubTitans ![GitHub all releases](https://img.shields.io/github/downloads/UnknownException/SubTitans/total)
## Unofficial patch for Submarine Titans

### Requirements
* Windows 11 or a Linux distribution with Wine or Proton
* Retail v1.1 or GOG v1.1

> Support for v1.0, Demo, and the Technology Demo is limited.
> These versions are not actively tested, and some features may be missing.

### Instructions
1. Copy & paste d3drm.dll, subtitans.dll and subtitans.ini into your Submarine Titans folder.
2. Open STConfig.exe and select 1280x1024.
3. Run the game.

> **For Steam users**: At least start the game once before applying this patch. steam_installscript.vdf will not be applied if you skip this step.

### Features
* Support for any resolution in-game (tested up to 3840x2160)
* OpenGL and Software rendering replacement for DirectDraw to improve compatibility and performance
* DInput replacement
* Improved scrolling
* Added mission skip cheat (Retail/GOG v1.1; workaround for progression bugs)
* Fixes various internal errors
* Fixes alt-tab crashes
* Fixes video issues
* Support for display scaling
* Palette color fix

### Known bugs
* Regions next to the in-game command panel aren't selectable/clickable.

## Question and Answers
### How to update Submarine Titans to version 1.1?
> Windows: https://steamcommunity.com/sharedfiles/filedetails/?id=2129291420 \
> Linux: See [PROTON-v1_0-v_1_1.md](PROTON-v1_0-v1_1.md)

### Why am I getting MSVCP140.dll errors?
> You're missing some libraries on your computer, you'll need to install the Visual C++ redistributable provided by Microsoft. \
> vc_redist.x86.exe ( https://aka.ms/vc14/vc_redist.x86.exe )

### I've got a problem in-game with my mouse and/or keyboard.
> Open SubTitans.ini and set CustomInput to *false*. \
> This will turn off the DInput reimplementation. 

### How can I use a custom or GOG's DDraw wrapper?
> Open SubTitans.ini and set Renderer to **1**.
> 
> Only the OpenGL renderer and Software renderer are tested while developing this patch. \
> Custom wrappers might introduce new issues, please validate if issues you observe also occur under the OpenGL/Software renderer.

### The Submarine Titans (technology) demo crashes with or without this patch.
> The support for the demo versions of Submarine Titans is bare minimum. Please don't report issues regarding the demo, it will be ignored.

### Why would I want to enable the experimental ASLR/DEP option?
> Many (older) applications have inherent vulnerabilities, and exposing them over the internet (such as in multiplayer modes) increases the attack vector by a huge margin.
> 
> By enabling **ASLR (Address Space Layout Randomization)** and **DEP (Data Execution Prevention)**, you tighten security by making the application's memory space less predictable. While this doesn't patch the vulnerabilities within the game itself, it makes them significantly harder for an attacker to successfully exploit

### How do I use the mission skip cheat?
> Load the mission you want to skip and write 'orbiton' without quotes in the chatbox. \
> Exit to the menu and select a random campaign to start the next mission.

### How do I uninstall the patch?
> Deleting SubTitans.dll will disable the in-game patches.
> 
> Do **NOT** delete d3drm.dll, the game will stop working without it.
