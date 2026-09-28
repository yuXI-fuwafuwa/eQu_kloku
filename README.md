# eQu Kloku

一个帮助专注学习的轻量计时 App。开始一段专注后，界面不会显示倒计时、进度或其他时间信息；你可以安心学习，让轻柔的提示音和结束铃声来提醒你。

## 构建与运行

支持 Linux、Windows 和 macOS 桌面环境。需要 C++17 编译器、CMake 和 raylib 6；CMake 会优先使用 raylib 的包配置，在没有包配置时尝试 pkg-config。中文字体已随项目提供，无需在目标系统单独安装。

Linux：

```sh
cmake -S . -B build
cmake --build build
./build/equ-kloku
```

macOS（先通过 Homebrew 安装 `cmake` 和 `raylib`）：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
open build/equ-kloku.app
```

Windows（Visual Studio 2022、CMake 和 vcpkg；将 `C:/vcpkg` 换成实际路径）：

```powershell
vcpkg install raylib:x64-windows-static
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static
cmake --build build --config Release
.\build\Release\equ-kloku.exe
```

各平台运行测试：

```sh
ctest --test-dir build --output-on-failure
```

Windows 使用 Visual Studio 等多配置生成器时，在 `ctest` 命令后加 `-C Release`。

推送代码后，GitHub Actions 的「Cross-platform build」工作流会尝试在 Windows x64、macOS Apple Silicon 和 macOS Intel 上构建并测试。成功运行的页面底部可下载对应的 ZIP 构建产物。Windows ZIP 包含 `bin` 和 `share` 目录，解压后需保持两者的相对位置。macOS ZIP 包含 `.app`；它使用 Homebrew 提供的 raylib 动态库，因此在其他 Mac 上运行前仍需安装 raylib。这些是 CI 构建产物，不是已签名或公证的正式安装包。

## 安装

Linux 构建后，可用 CMake 将程序和字体分别安装到 `/usr/bin/equ-kloku` 与 `/usr/share/equ-kloku`：

```sh
sudo cmake --install build --prefix /usr
equ-kloku
```

安装目标也支持传统的 `make install`：使用 Makefiles 生成器并在配置时指定 `-DCMAKE_INSTALL_PREFIX=/usr`，再运行 `sudo make -C build install`。不指定安装前缀时，CMake 默认安装到 `/usr/local/bin`。

macOS 可使用 `cmake --install build --prefix "$HOME"` 安装到 `~/Applications/equ-kloku.app`。Windows 的安装目标会把程序放在安装前缀的 `bin`，字体和许可文件放在 `share/equ-kloku`。

提示音由程序生成，无需额外音频文件。如果音频设备不可用，界面仍能正常计时，但不会播放提醒。

## 使用

1. 在「这次想专注多久？」中输入 1–999 分钟，点击圆形「开始专注」按钮，也可以按 Enter。
2. 专注过程中，可点「稍作休息」暂停，再点「继续专注」恢复；点「结束本次专注」则直接放弃本次记录。
3. 完成后会听到铃声，界面显示「这段专注完成啦」。完成次数此时立即保存，点「好的」返回首页。

程序会从 −10 到 +10 分钟均匀抽取一个整数，加入你输入的时长；若结果不足 1 分钟，则按 1 分钟计。它还会在实际时长的 65%–75% 之间随机选一个提醒点，播放一次轻柔提示音。暂停期间不会推进计时；关闭应用时未完成的专注不计入完成次数。

完成次数按系统惯例保存在：

- Linux：`$XDG_DATA_HOME/equ-kloku/progress.txt`，未设置时为 `~/.local/share/equ-kloku/progress.txt`。
- macOS：`~/Library/Application Support/equ-kloku/progress.txt`。
- Windows：`%APPDATA%\equ-kloku\progress.txt`。

随包字体为 Adobe 的 Source Han Sans CN Regular，遵循 SIL Open Font License 1.1；版权声明与许可条款见 [assets/OFL.txt](assets/OFL.txt)。
