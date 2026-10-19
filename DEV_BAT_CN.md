# dev.bat 使用说明

`dev.bat` 是仓库根目录下的**编译烧录入口**。它只做分发，实际动作全部交给仓库里已有的工具：

| 动作 | 实际调用 |
|---|---|
| 编译 / 清理 | `esp-idf\tools\idf.py`（`build` / `fullclean` / `clean` / `app-flash`） |
| 全烧录 | esp-idf 自带的 `esptool`，地址从 `apps\build\flash_args` 读取 |

工程根目录是 `apps\`（`project(vb701n)`），所以脚本里所有 `idf.py` 调用都带 `-C apps`。

## 用法

```text
dev.bat build                    增量编译            idf.py build
dev.bat rebuild                  全量编译            删掉 apps\build 后 build
dev.bat clean                    清理编译缓存        删掉整个 apps\build
dev.bat clean obj                只清对象文件        idf.py clean（保留 sdkconfig）
dev.bat flash -p COMx            下载：只烧应用层     idf.py app-flash → 0x20000
dev.bat flashall -p COMx         全烧录：基底+应用    esptool 分两次写，不合并
dev.bat flashall -p COMx erase   全烧录前先全片擦除  会清掉 nvs 与已保存的 WiFi 配网信息
dev.bat help                     显示帮助
```

- **串口必填** `-p COMx`；也可以在环境变量 `DEV_PORT` 里给（脚本里不写死 COM3）。
  查看本机串口：PowerShell 里执行 `[System.IO.Ports.SerialPort]::GetPortNames()`。
- `下载` 与 `全烧录` 的区别：前者只写应用分区（日常改代码后的快速迭代），后者先把官方基底写完再写应用。

## 全烧录的地址表

`flashall` **不做合并镜像**，按地址分两次写：先基底、后应用。地址不是写死的，是从 IDF 生成的 `apps\build\flash_args` 里解析出来的，当前分区表（`apps\partitions.csv`）对应：

| 文件 | 地址 |
|---|---|
| `bootloader/bootloader.bin` | `0x0` |
| `partition_table/partition-table.bin` | `0x10000` |
| `ota_data_initial.bin` | `0x1d000` |
| `phy_init_data.bin` | `0x1f000` |
| `vb701n.bin`（应用） | `0x20000`（ota_0） |

脚本会在写之前把解析到的地址打印出来，方便核对。

## 脚本内可改的项（文件头部）

| 变量 | 默认 | 说明 |
|---|---|---|
| `IDF_TOOLS_PATH` | `C:\esp32\esp32_combo\.espressif` | 工具链/python 环境所在目录；该目录下没有 `python_env` 时回退 `%USERPROFILE%\.espressif` |
| `CHIP` | `esp32s3` | esptool 的芯片参数 |
| `BAUD` | `921600` | esptool 烧录波特率 |
| `APP_BIN` | `vb701n.bin` | 应用固件名（工程改名时同步这里） |

## 为什么自带环境，而不是用仓库的 export

仓库的 `export.bat` / `WINDOWS_BUILD_CN.md` 里的 `export.ps1` 都依赖 `idf_tools.py export` 正常激活。**本机 `.espressif` 里 `openocd-esp32` / `riscv32-esp-elf-gdb` 的版本与该 IDF 分支要求的不一致**，激活会直接报：

```text
ERROR: tool openocd-esp32 has no installed versions.
* Establishing a new ESP-IDF environment ... FAILED
error: Activation script failed
```

所以 `dev.bat` 在脚本内直接设置环境（照搬可用的那几项），不走 export：

- `IDF_PATH` / `ADF_PATH`：**必须用正斜杠**（上游已在 `apps/main/CMakeLists.txt` 里用 `file(TO_CMAKE_PATH)` 兜底反斜杠，但脚本仍按正斜杠设置）
- `IDF_TOOLS_PATH`、`IDF_PYTHON_ENV_PATH`
- `PATH` 前置交叉工具链与构建工具：`xtensa-esp-elf` / `cmake` / `ninja` / `esp32ulp-elf`
- `ESP_ROM_ELF_DIR`

工具链装到别处时，只需改脚本头部的 `IDF_TOOLS_PATH`。若你已按 `WINDOWS_BUILD_CN.md` 把工具链补装齐（`idf_tools.py install`），也可以继续用 `export.ps1`，两种方式不冲突。

## 为什么不直接用 idf.py fullclean

`fullclean` 除了删 build 目录，还会执行 `remove_managed_components`：核验并删掉 `apps\managed_components\78__esp-opus`（该组件是提交在仓库里的）。这里有两个坑：

1. **哈希核验会失败**。系统级 `core.autocrlf=true`（Git for Windows 默认）会把检出的文本文件转成 CRLF，而组件管理器是拿注册表下载时的 **LF 原始字节**算哈希的，于是报：

   ```text
   ERROR: Some components in the "managed_components" directory were modified on the disk ...
   78__esp-opus: Hash of the file "releases.sha2" ... does not match expected hash 328426bd...
   ```

   本地修复（不进仓库）：`.git/info/attributes` 里加一行 `apps/managed_components/** -text`，然后把该目录重新检出成 LF：

   ```powershell
   Remove-Item -Recurse -Force apps\managed_components\78__esp-opus
   git checkout -- apps/managed_components/78__esp-opus
   git ls-files --eol apps/managed_components/78__esp-opus/releases.sha2   # 期望 i/lf w/lf
   ```

   团队级的正解是在仓库根加 `.gitattributes`（同样写 `apps/managed_components/** -text`），这属于上游要改的东西。

2. **即使核验通过**，`fullclean` 也会把这份提交在仓库里的组件删掉，下次编译再从组件市场重新下载，`git status` 会出现 700 个文件的删改噪声。

所以 `dev.bat` 的 `clean` / `rebuild` 改成**直接删 `apps\build`**（等价于"清编译缓存"，完全不碰 `managed_components`）。

## 已知注意事项

1. **本脚本必须保持纯 ASCII（英文注释 + 英文输出）**。本机 cmd 会话码页是 `65001`、系统 ACP 是 `936`，文件里只要出现中文（不管存成 UTF-8 还是 GBK），在另一种码页的 cmd 下都会被解码成碎片字节，连 `rem` 行都会被拆断，报"不是内部或外部命令"。另外输出文字里不要带圆括号 —— `( )` 在 `if ( ... )` 块内会被 cmd 当成块分隔符。
2. **esptool 4.12 的写法**：操作名和选项是下划线 —— `write_flash`、`--flash_mode`、`--flash_freq`、`--flash_size`；`-p` 在 esptool 里是 `--no-progress`（不是串口），串口要用 `--port`。手敲命令时最容易踩这个。
3. `rebuild` / `clean`（不带 `obj`）会删掉整个 `apps\build`，**clangd 用的 `compile_commands.json` 也随之消失**，重新 `dev.bat build` 即恢复。
4. 首次编译或换分支后，先 `dev.bat build` 再烧录；`flashall` 依赖 `apps\build\flash_args`，没编译过会提示先编译。
5. 失败即返回：脚本里每一步都检查退出码，失败会直接停在出错的那一步并给出非零退出码，不会继续往下烧。
