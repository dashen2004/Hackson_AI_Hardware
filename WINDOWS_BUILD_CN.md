# Windows 下编译

Cursor 和 Windows Terminal 默认是 PowerShell。本仓库的 `export.bat` 在 PowerShell 里跑不起来，要用 `export.ps1`。

## 激活环境

每个新开的 PowerShell 窗口都要做一次。先 `cd` 到仓库根目录，就是和 `esp-idf`、`apps`、`tools` 同一层的那个目录。

`Set-ExecutionPolicy` 只对当前窗口生效，不改系统策略。

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass -Force
$env:ADF_PATH = (Resolve-Path .).Path
$env:IDF_PATH = Join-Path $env:ADF_PATH "esp-idf"
. .\esp-idf\export.ps1
python .\tools\adf_install_patches.py apply-patch
```

比如仓库在 `D:\Work\Proj\esp32_combo`，先：

```powershell
cd D:\Work\Proj\esp32_combo
```

再跑上面那几行。跑完之后两个变量是：

```text
ADF_PATH = D:\Work\Proj\esp32_combo
IDF_PATH = D:\Work\Proj\esp32_combo\esp-idf
```

别人把自己的仓库路径换上去就行，不用改命令本身。

这两个变量必须是绝对路径。写成 `.\esp-idf` 的话，下一步 `cd .\apps` 之后 CMake 会到 `apps` 下面去找 IDF，找不到。

`. .\esp-idf\export.ps1` 前面的点和空格不能省。激活成功会看到 `Identifying shell ... powershell.exe`，然后 `idf.py` 就能用了。

进应用目录编译：

```powershell
cd .\apps
idf.py build
```

芯片型号从 `apps/sdkconfig` 读。这个工程会猜成 `esp32s3`。
