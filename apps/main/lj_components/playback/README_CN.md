# 应用层资源播放

这个目录位于 `apps/main/lj_components/playback`，封装了 `resource_playback.md` 使用到的 QUMI `qm_player` 和
`qm_res_mgr` 功能，提供：

- 基于已有共享 `qm_audio` 创建播放器；
- URL 播放和注入本地 source 的文件播放；
- 通过当前 MQTT 元数据通道查询资源列表、分类和详情；
- 通过应用自己的回调接收资源下载数据；
- 暂停、恢复、停止、状态查询和销毁。

`playback_init()` 需要调用方传入 decoder，以及可选的本地文件 source。
组件不提供具体音频解码器，也不创建 `qm_audio`；这两个对象由调用方持有，
直到 `playback_deinit()` 返回后才能释放。

`apps/main/CMakeLists.txt` 已把 `lj_components/playback/playback.c` 加入编译，
并添加对应的头文件路径。启动流程是否调用 `playback_init()`，以当前 `main.c`
的实际初始化顺序为准。

资源列表和详情查询是同步调用，应放在应用 worker 中执行，不能在 MQTT 接收回调
里等待。`playback_play_*()` 只表示播放命令已经入队，实际开始、结束或错误要通过
播放器事件和状态接口确认。

当前工程将 `CONFIG_QM_PLAYER_CLOUD_SUPPORT` 关闭，MQTT 的 `player` 云控命令仍由
应用层自行路由；该组件只负责播放、资源查询和下载，不会重复注册 `player` 方法。
