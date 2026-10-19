# 应用层资源播放

这个目录位于 `apps/main/lj_components/playback`，封装了 `resource_playback.md` 使用到的 QUMI `qm_player` 和
`qm_res_mgr` 功能，提供：

- 基于已有共享 `qm_audio` 创建播放器；
- URL 播放和注入本地 source 的文件播放；
- 通过当前 MQTT 元数据通道查询资源列表、分类和详情；
- 通过应用自己的回调接收资源下载数据；
- 暂停、恢复、停止、状态查询和销毁。
- 接收应用层 `player/play` 云控请求，按先 guideWords、后资源 PCM 的顺序播放；
- 在资源播放期间暂停采集并关闭 agent 上行，结束后恢复共享音频路径。

`playback_init()` 需要调用方传入 decoder，以及可选的本地文件 source。当前应用
提供 `resource_pcm_decoder`，只支持云控 `audioType=pcm`（16 kHz、单声道、16-bit
原始 PCM）；组件不创建 `qm_audio`，共享音频由调用方持有。

`apps/main/qm_config/qm_config.h` 中的 `CONFIG_LJYD_PLAYBACK_SUPPORT` 控制应用层
资源播放组件。设为 `1` 时，`apps/main/CMakeLists.txt` 编译 `playback.c` 和
`resource_pcm_decoder.c`，`main.c` 在 `voice_start()` 后执行 `playback_init()`；
设为 `0` 时，同时移除组件源文件、云控命令路由、唤醒停止调用和启动初始化。

资源列表和详情查询是同步调用，应放在应用 worker 中执行，不能在 MQTT 接收回调
里等待。`playback_play_*()` 只表示播放命令已经入队，实际开始、结束或错误要通过
播放器事件和状态接口确认。

当前工程将 `CONFIG_QM_PLAYER_CLOUD_SUPPORT` 关闭，MQTT 的 `player/play` 云控命令
由应用层路由到 playback worker；组件不会修改或注册 QUMI 内部云控实现。
