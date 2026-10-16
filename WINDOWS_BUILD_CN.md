# Windows 编译说明

本仓库包含 ESP-ADF、ESP-IDF 和 `apps` 应用。Windows 下先初始化 ESP-IDF 环境，再进入 `apps` 目录编译。

## PowerShell（推荐）

在仓库根目录执行。每个新开的 PowerShell 窗口都需要初始化一次：

```powershell
Set-Location D:\Code\esp32_combo
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass -Force
.\export.ps1
```

`export.ps1` 会自动设置 `ADF_PATH` 和 `IDF_PATH`，加载 ESP-IDF 环境，并应用仓库中的 IDF 补丁。成功后应看到：

```text
ESP-IDF environment setup completed successfully.
```

然后在同一个 PowerShell 窗口中编译：

```powershell
Set-Location .\apps
idf.py build
```

脚本需要在当前窗口中执行，不能在一个窗口初始化环境后换另一个窗口运行 `idf.py`。`Set-ExecutionPolicy -Scope Process` 只对当前窗口生效，不会修改系统策略。

如果当前 PowerShell 已允许本地脚本运行，可以省略 `Set-ExecutionPolicy`。

## cmd.exe

在仓库根目录打开 `cmd.exe`，执行：

```bat
cd /d D:\Code\esp32_combo
call export.bat
cd /d apps
idf.py build
```

在批处理脚本中调用 `export.bat` 时应使用 `call`，以便返回后继续执行编译命令。PowerShell 中应使用 `export.ps1`；直接运行 `export.bat` 时，环境变量只存在于它启动的子 `cmd.exe` 进程中。

## VS Code ESP-IDF 插件

可以使用 VS Code 的 ESP-IDF 插件编译和烧录：

1. 用 VS Code 打开 `D:\Code\esp32_combo\apps`，该目录是 ESP-IDF 工程目录。
2. 确保 ESP-IDF 插件已完成工具链配置，并使用本仓库的 ESP-IDF 目录：`D:\Code\esp32_combo\esp-idf`。新版插件通过 Espressif Install Manager 配置，旧版可使用 `ESP-IDF: Configure ESP-IDF extension`。
3. 当前 `apps\sdkconfig` 已配置目标芯片为 `esp32s3`；串口选择实际连接的 `COM` 端口。
4. 使用插件的 Build、Flash 和 Monitor 命令或状态栏按钮完成编译、烧录和串口监视。

首次使用新克隆的仓库时，先在 PowerShell 中初始化一次环境，确保 ADF 补丁已经应用：

```powershell
Set-Location D:\Code\esp32_combo
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass -Force
.\export.ps1
```

插件构建需要能读取 `ADF_PATH`。可从上面已经初始化的 PowerShell 窗口启动 VS Code：

```powershell
code .\apps
```

如果 VS Code 已经在运行，请先完全退出后再从该终端启动，确保它继承 `ADF_PATH` 等环境变量。也可以在插件的自定义环境变量设置 `idf.customExtraVars` 中添加 `ADF_PATH`，值为仓库根目录的绝对路径。

也可以在 `apps\.vscode\settings.json` 中写入：

```json
{
    "idf.customExtraVars": {
        "ADF_PATH": "D:\\Code\\esp32_combo",
        "IDF_PATH": "D:\\Code\\esp32_combo\\esp-idf"
    }
}
```

如果仓库路径不同，只替换 `ADF_PATH` 的值即可。

## 手动烧录和监视

先按前文初始化当前 PowerShell 窗口，再执行以下命令。将 `COM7` 替换成实际串口：

```powershell
Set-Location D:\Code\esp32_combo\apps
idf.py -p COM7 flash monitor
```

退出串口监视器使用 `Ctrl+]`。

## 常见问题

### PowerShell 提示无法识别 idf.py

通常是当前窗口尚未初始化环境，或者在 PowerShell 中运行了 `export.bat`。回到仓库根目录执行 `.\export.ps1`，然后在同一窗口进入 `apps` 编译。

### 仓库不在 D:\Code\esp32_combo

将命令中的仓库路径替换成实际路径。路径包含空格时使用引号，例如：

```powershell
Set-Location "D:\My Projects\esp32_combo"
```

### 已设置其他 IDF_PATH

根目录的 `export.ps1` 会保留已有的 `IDF_PATH`。如果当前窗口指向另一个 ESP-IDF，请先明确指定本仓库版本：

```powershell
Set-Location D:\Code\esp32_combo
$env:IDF_PATH = Join-Path (Get-Location).Path "esp-idf"
.\export.ps1
```

## 当前工程信息

- 目标芯片：`esp32s3`（来自 `apps\sdkconfig`）
- 工程目录：`apps`
- 构建输出目录：`apps\build`
- 应用名：`vb701n`
- Opus 组件：`78/esp-opus`（managed component）
