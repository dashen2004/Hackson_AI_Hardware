<div align="center">

# AI + 硬件黑客松项目

项目资料从[文档索引](docs/README.md)开始阅读；具体产品场景与方案待确认。

📚 [文档索引](docs/README.md)　·　🚀 [首次获取](#first-clone)　·　🔄 [已有工作区](#existing-workspace)　·　🛠️ [Windows 工具链](#windows-toolchain)

</div>

---

<a id="first-clone"></a>

**灵机 XJWB03 开发板 SDK** 位于 `src/firmware/`，通过 Git 子模块固定上游版本。**首次获取本项目时请递归克隆：**

```powershell
git clone --recurse-submodules https://github.com/dashen2004/Hackson_AI_Hardware.git
```

<a id="existing-workspace"></a>

**已有本项目工作区时，在项目根目录执行：**

```powershell
git submodule update --init --recursive
```

> 代码来源、目录入口、Gitee SSH 获取方式和后续编译步骤见[固件代码获取与运行说明](docs/开发/固件代码获取与运行说明.md)。

---

<a id="windows-toolchain"></a>

**Windows 工具链安装与环境激活（项目根目录的 PowerShell）：**

```powershell
.\tools\Install-FirmwareTools.ps1
. .\tools\Enter-Firmware.ps1
idf.py -C src/firmware/apps build
```

> 版本与验证范围见[Windows 工具链安装记录](docs/开发/Windows工具链安装记录.md)。
