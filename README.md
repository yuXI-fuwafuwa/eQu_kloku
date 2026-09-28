# eQu Kloku

一个帮助专注学习的轻量计时 App。开始一段专注后，界面不会显示倒计时、进度或其他时间信息；你可以安心学习，让轻柔的提示音和结束铃声来提醒你。

## 构建与运行

首版面向 Linux，需要 C++17 编译器、CMake、pkg-config、raylib 6，以及思源黑体 CN（或本机提供的 Noto Sans CJK、Maple Mono NF CN）。在当前环境中运行：

```sh
cmake -S . -B build
cmake --build build
./build/equ-kloku
```

运行测试：

```sh
ctest --test-dir build --output-on-failure
```

## 安装

构建后，可用 CMake 的安装命令将程序放到 `/usr/bin/equ-kloku`：

```sh
sudo cmake --install build --prefix /usr
equ-kloku
```

安装目标也支持传统的 `make install`：使用 Makefiles 生成器并在配置时指定 `-DCMAKE_INSTALL_PREFIX=/usr`，再运行 `sudo make -C build install`。不指定安装前缀时，CMake 默认安装到 `/usr/local/bin`。

提示音由程序生成，无需额外音频文件。如果音频设备不可用，界面仍能正常计时，但不会播放提醒。

## 使用

1. 在「这次想专注多久？」中输入 1–999 分钟，点击圆形「开始专注」按钮，也可以按 Enter。
2. 专注过程中，可点「稍作休息」暂停，再点「继续专注」恢复；点「结束本次专注」则直接放弃本次记录。
3. 完成后会听到铃声，界面显示「这段专注完成啦」。完成次数此时立即保存，点「好的」返回首页。

程序会从 −10 到 +10 分钟均匀抽取一个整数，加入你输入的时长；若结果不足 1 分钟，则按 1 分钟计。它还会在实际时长的 65%–75% 之间随机选一个提醒点，播放一次轻柔提示音。暂停期间不会推进计时；关闭应用时未完成的专注不计入完成次数。

完成次数保存在 `$XDG_DATA_HOME/equ-kloku/progress.txt`；若未设置 `XDG_DATA_HOME`，则保存在 `~/.local/share/equ-kloku/progress.txt`。
