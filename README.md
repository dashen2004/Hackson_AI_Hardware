<div align="center">

# AI + 硬件黑客松项目

项目资料从[文档索引](docs/README.md)开始阅读；当前产品基线为 Ticko，需求与开发阶段见[产品主文档](docs/产品/ticko最小可交互文档.md)。

📚 [文档索引](docs/README.md)　·　📂 [仓库目录](#user-content-repository-structure)　·　🚀 [首次获取](#user-content-first-clone)　·　🔄 [已有工作区](#user-content-existing-workspace)　·　🛠️ [Windows 工具链](#user-content-windows-toolchain)

</div>

---

<a id="repository-structure"></a>

## 📂 仓库与文档目录

```text
Hackson_AI_Hardware/
├── assets/
│   └── platform/                    # 平台与配网资料
├── .agents/
│   └── skills/                      # 本项目可复用的代理技能
├── docs/                            # 项目文档
│   ├── meetings/                    # 会议纪要
│   ├── 产品/                         # Ticko 当前产品基线与配套图片
│   ├── 创意/                         # 产品创意与需求
│   ├── 开发/
│   │   └── AI插件/                   # 平台插件与集成说明
│   ├── 硬件/
│   │   ├── 开发板/
│   │   │   └── 官方资料/
│   │   ├── 摄像头/
│   │   ├── MPU6050/
│   │   ├── WS2812灯圈/
│   │   ├── 模组/
│   │   ├── 舵机/
│   │   └── 震动马达/
│   ├── 赛事/                         # 参赛与方案资料
│   ├── 赛题/                         # 赛题要求
│   ├── README.md                    # 文档索引
│   └── Ticko一页方案说明.doc
├── src/
│   ├── control-panel/               # Ticko H5 控制面板
│   └── firmware/                    # 灵机 XJWB03 SDK Git 子模块
├── tools/
│   ├── camera/                      # 摄像头串口预览与截图
│   ├── firmware/                    # Windows 固件环境脚本
│   ├── platform/                    # 平台配置与智能体离线预演
│   └── tests/                       # 按摄像头、固件、平台分类的回归测试
├── .gitmodules
├── AGENTS.md                        # 项目协作指南
├── README.md                        # GitHub 项目首页
└── STATUS.md                        # 项目状态台账
```

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
.\tools\firmware\Install-FirmwareTools.ps1
. .\tools\firmware\Enter-Firmware.ps1
idf.py -C src/firmware/apps build
```

> 版本与验证范围见[Windows 工具链安装记录](docs/开发/Windows工具链安装记录.md)。
