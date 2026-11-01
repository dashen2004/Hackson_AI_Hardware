# 本地工具

工具按运行对象和用途分类，命令均从仓库根目录执行。

目录分组：

- camera：摄像头串口预览、单帧导出及依赖。
- firmware：Windows 固件工具安装与环境激活。
- platform：产品本机配置及智能体离线预演。
- tests/camera：摄像头预览协议测试。
- tests/firmware：固件业务 C 路径的主机回归测试。
- tests/platform：本机配置与离线预演测试。

摄像头工具的安装与运行步骤见[实时坐姿检测说明](../docs/开发/摄像头实时坐姿检测运行说明.md)。固件安装和构建流程见[固件代码获取与运行说明](../docs/开发/固件代码获取与运行说明.md)。

常用入口：

- 摄像头预览：python tools/camera/camera_preview.py --port COM7
- 单帧截图：python tools/camera/camera_snapshot.py --port COM7 --output frame.png
- 安装固件工具：.\tools\firmware\Install-FirmwareTools.ps1
- 回归测试：python tools/tests/分类/测试文件.py

具体测试文件和验收范围见各专题开发记录。主机测试不替代固件构建、设备烧录或实物验收。
