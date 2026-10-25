# Windows 工具链安装记录

## 适用工程

- 安装日期：2026-10-22。
- SDK：`src/firmware/`，提交 `bebdc80209f1b3c2611f9602577be1c0d1ac4b96`。
- ESP-IDF：SDK 内置的 5.4.1，目标芯片 ESP32-S3。
- 开发板资料型号：XJWB03-DevKIT-V1 + LJX40-Audio-KIT-V1；实际套件型号待核对。
- 本次为电脑端工具链安装，不涉及接线、设备供电或烧录；硬件连接与供电要求见[开发板使用文档](../硬件/开发板/灵机XJWB03标准开发板使用文档.md)。

## 本机环境与版本

工具目录：`C:\Users\asus\.espressif`。Python 虚拟环境：`C:\Users\asus\.espressif\python_env\idf5.4_py3.12_env`。SDK 源码仍使用项目内置目录，没有另行下载其他 ESP-IDF 版本。

| 工具 | 版本 | 说明 |
| --- | --- | --- |
| Git for Windows | 2.47.0.windows.1 | 复用原有安装 |
| Python | 3.12.10 | 复用原有安装，另建 ESP-IDF 独立环境 |
| Xtensa GCC | esp-14.2.0_20241119 | ESP32-S3 主核交叉编译器 |
| RISC-V GCC | esp-14.2.0_20241119 | 官方 ESP32-S3 工具集合中的辅助工具链 |
| Xtensa GDB | 14.2_20240403 | 调试器 |
| ESP32 ULP 工具链 | 2.38_20240113 | 低功耗协处理器工具 |
| CMake | 3.30.2 | 构建配置 |
| Ninja | 1.12.1 | 构建执行 |
| OpenOCD | v0.12.0-esp32-20241016 | 调试服务 |
| idf-exe | 1.0.3 | Windows IDF 命令入口 |
| ccache | 4.10.2 | 编译缓存 |
| dfu-util | 0.11 | DFU 下载工具 |
| ESP ROM ELFs | 20241011 | ROM 调试符号 |

Python 核心依赖由官方 `espidf.constraints.v5.4.txt` 约束安装，包含 esptool、串口监视器和组件管理器。未启用可选的 esp-clang 或 QEMU。

## 安装与日常使用

在项目根目录执行：

```powershell
# 首次安装或补齐工具。
.\tools\Install-FirmwareTools.ps1

# 每个新 PowerShell 窗口激活一次；点号后有空格。
. .\tools\Enter-Firmware.ps1
idf.py --version

# 编译项目。
idf.py -C src/firmware/apps build
```

如执行策略阻止本地脚本，可先运行 `Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass -Force`，仅影响当前窗口。安装器和 PowerShell 激活入口位于主项目 `tools/`。激活只改变当前进程环境，关闭终端后失效。

2026-10-23 已修复 SDK 的 `dev.bat`，通过官方 `activate.py --export --shell cmd.exe` 自动定位 Python 环境及工具版本，移除了上游作者电脑的固定路径。现在也可从项目根目录直接运行：

```powershell
.\src\firmware\dev.bat build
```

该命令在子进程中自动激活环境，不需要先运行 PowerShell 激活脚本。两个入口均不调用上游 ADF 补丁脚本。`dev.bat` 的修复已于 2026-10-25 提交到团队 GitHub 仓库的 `codex/firmware-sdk` 分支；2026-10-22 安装阶段 SDK 源码干净的记录仅描述当时状态。团队获取方式见[固件代码获取与运行说明](固件代码获取与运行说明.md)。

2026-10-23 在未预先激活环境的 PowerShell 中执行 `.\dev.bat build`，成功定位 Python 3.12.10 环境，增量构建退出码为 0，输出 `[dev] build done`；应用与 bootloader 的分区大小检查均通过。本次复用 2026-10-22 已完成的编译产物，未重新执行全量构建，未烧录硬件。

### 下载网络

安装脚本默认使用 Espressif 官方 GitHub 发布包镜像 `dl.espressif.com/github_assets`，依赖版本约束从 Espressif 官方下载。本次 PyPI 文件下载缓慢，安装时仅在进程内设置了清华镜像，没有写入全局 pip 配置。如再次遇到相同情况，可执行：

```powershell
$previousPipIndex = $env:PIP_INDEX_URL
try {
    $env:PIP_INDEX_URL = 'https://pypi.tuna.tsinghua.edu.cn/simple'
    .\tools\Install-FirmwareTools.ps1
} finally {
    $env:PIP_INDEX_URL = $previousPipIndex
}
```

## 验证结果

| 检查 | 结果 |
| --- | --- |
| 官方工具安装检查 `idf_tools.py check` | 通过，ESP32-S3 所需工具版本已安装 |
| 官方 Python 依赖检查 `check-python-dependencies` | 通过，满足 ESP-IDF 5.4 官方约束 |
| PowerShell 7 激活 | 通过，使用 `idf5.4_py3.12_env` |
| Windows PowerShell 5.1 激活 | 通过，使用 `idf5.4_py3.12_env` |
| `idf.py --version` | ESP-IDF v5.4.1 |
| CMake / Ninja / Xtensa GCC | 命令可执行，版本分别为 3.30.2 / 1.12.1 / 14.2.0 |
| SDK 应用构建 | 通过：`idf.py` 完成配置及依赖解析，单目标重编通过后，以 `ninja -j 8 all` 完成完整构建，退出码 0 |
| 构建产物 | 已生成 `apps/build/vb701n.bin`、`vb701n.elf`、`bootloader/bootloader.bin` 和分区表 |
| 应用分区大小检查 | 通过：应用大小 `0x2f8f70` 字节，最小应用分区 `0x3f0000` 字节，约 25% 空余 |
| SDK 源码 | 编译后 `git -C src/firmware status --short` 为空，未修改上游跟踪文件 |
| PowerShell 脚本语法与文档相对链接 | 通过 |
| 硬件验证 | 未执行设备接线、供电、烧录、串口通信、配网或实物功能验证 |

执行 `idf.py --version` 时会先显示 `esp-idf/.git` 不存在的提示，再从源码读出正确的 `v5.4.1`。这是因为当前 SDK 将 ESP-IDF 作为普通目录内置，而非独立 Git 仓库；该提示没有阻止本次环境激活和 CMake 配置。

首次 Ninja 默认并发构建曾在 FreeRTOS 的 `idf_additions.c` 编译时报告读取 `sys/syslimits.h` 的 `Invalid argument`。同一目标单独重编译通过，未修改源码；具体原因尚未确认。本次随后在已经完成 CMake 配置的构建目录内将并发降为 8，完整构建通过，错误未再出现：

```powershell
. .\tools\Enter-Firmware.ps1
ninja -C src/firmware/apps/build -j 8 all
```

此命令用于已有 `build.ninja` 的构建目录；首次配置或修改工程配置后，先使用 `idf.py -C src/firmware/apps build`。

本次构建仍有上游代码的编译警告，例如 `qm_pwm.c` 的类型范围比较与未使用变量；未将警告清理扩展为本次安装任务。生成固件仅证明电脑端构建通过，PID、设备密钥、实际开发板型号以及运行时行为仍待确认。

返回：[固件代码获取与运行说明](固件代码获取与运行说明.md) · [文档索引](../README.md)。
