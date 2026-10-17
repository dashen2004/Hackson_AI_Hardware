# 应用层资源播放

本文档描述当前仓库 `apps/main/lj_components/playback` 的实际实现。该目录是一个应用层
包装，不是 QUMI 的通用资源播放组件。

当前代码提供两类能力：

- 使用 `qm_player` 播放 URL 或由应用层 source 适配器提供的本地文件；
- 使用 `qm_res_mgr` 通过当前配置的 MQTT 元数据通道查询资源，并把资源详情
  转换成 `qm_player` 播放请求。

资源播放和语音对话是两条不同的音频路径。资源播放使用下面的链路：

```text
资源元数据 MQTT 查询
        |
        v
qm_res_mgr --获取资源 URL--> qm_player
                              |
                 +------------+------------+
                 |                         |
              HTTP URL                 本地文件
                 |                         |
              内置 HTTP source       应用 source_if
                 +------------+------------+
                              |
                           decoder_if
                              |
                           qm_audio
                              |
                         音频驱动/喇叭
```

当前工程的语音对话使用 `qm_agent` 和 Agora transport。日志中的
`dialog=speaking`、`downlink first frame`、TTS 和 Opus 解码属于
`qm_agent -> qm_audio` 路径，不会自动进入本文件的 `playback_*()` 接口。

## 当前工程状态

`apps/main/CMakeLists.txt` 已经把 `lj_components/playback/playback.c` 加入编译，并提供了
QUMI 的 `qm_player`、`qm_res_mgr` 头文件路径。但是当前
`apps/main/main.c` 的启动顺序是：

```text
voice_start()
  -> ljyd_cloud_init()
  -> ljyd_provision_init()
  -> agent_app_init()
```

启动流程目前没有调用 `playback_init()`，应用层也没有提供
`qm_player_decoder_if_t` 解码器适配器。因此资源播放器目前只是已经编译进
工程的应用层接口，尚未被启动流程真正创建。

这也意味着：

- 直接调用 `playback_play_url()`、`playback_play_file()` 或
  `playback_play_id()` 前，必须先完成 `playback_init()`；否则会返回
  `-QM_EINIT`；
- 当前语音唤醒回调会调用 `playback_stop()`，但播放器尚未初始化时该调用
  会返回 `-QM_ENOENT`，随后仍会通过 `qm_agent_interrupt()` 停止本地 TTS；
- 当前工程没有自动注册 MQTT `method=player` 的云控路由。

## 配置边界

当前 `apps/main/qm_config/qm_config.h` 的相关配置为：

```c
#define CONFIG_QM_PLAYER_SUPPORT 1
#define CONFIG_QM_PLAYER_DECODER_SUPPORT 1
#define CONFIG_QM_PLAYER_CLOUD_SUPPORT 0

#define CONFIG_QM_RES_MGR_SUPPORT 1
#define CONFIG_QM_RES_MGR_HTTP_SUPPORT 0
#define CONFIG_QM_RES_MGR_MQTT_SUPPORT 1
#define CONFIG_QM_RES_MGR_PAGE_SIZE 10
#define CONFIG_QM_RES_MGR_MQTT_TIMEOUT_MS_DEFAULT 15000
```

含义如下：

- `qm_player` 和解码器边界已启用，但具体解码器仍由应用层提供；
- `qm_player_cloud` 云控适配已关闭，QUMI 不会自动接管 `player` 方法；
- `qm_res_mgr` 的列表、分类、子分类和详情查询使用 MQTT 元数据通道；
- 资源文件本身仍由返回的 URL 通过 HTTP 下载；
- MQTT 元数据模式不使用 `qm_res_mgr_set_env()`，调用它会返回
  `-QM_ENOSYS`。`DEV/TEST/PROD` 环境切换只适用于 HTTP 元数据模式。

固定容量、队列长度、超时等构建参数放在 `qm_config`/Kconfig；DID、资源包
名称、资源 ID、MQTT handle 和资源 URL 等运行时数据必须通过 API 参数传入。

## 播放器初始化

播放器由 `playback_init()` 创建：

```c
int playback_init(qm_audio_handle_t audio,
                  const qm_player_decoder_if_t *decoder_if,
                  const qm_player_source_if_t *source_if);
```

参数要求：

| 参数 | 要求 |
| --- | --- |
| `audio` | 必须是已经创建好的共享 `qm_audio` handle，当前工程可由 `voice_get_audio()` 获取 |
| `decoder_if` | 必须提供，负责压缩音频解码和 PCM frame 输出 |
| `source_if` | 播放 `QM_PLAYER_LOCATION_FILE` 时必须提供；只播放 URI 时可以为 `NULL` |

该函数只保存接口指针并创建 `qm_player` worker，不创建 `qm_audio`，也不创建
解码器。调用方必须保证 `audio`、`decoder_if` 和 `source_if` 在
`playback_deinit()` 返回前一直有效。

`qm_player_decoder_if_t` 的 `open()` 会收到一个带有 `read()`、输出格式和
frame sink 的解码会话；解码器应把 PCM frame 推送给 `frame_sink.push()`。
解码器停止时必须中断自己的读取和任务，`close()` 负责释放解码器会话。

当前仓库里的 `apps/main/agent/private/opus.c` 是 Agora 语音帧的 Opus 编解码
辅助代码，不是 `qm_player_decoder_if_t` 实现，不能直接作为
`playback_init()` 的第二个参数。

## 播放 URL

```c
#include "playback.h"

int ret = playback_play_url("https://example.com/song.mp3",
                            QM_PLAYER_RESOURCE_TYPE_MP3,
                            0U,  /* 文件大小未知 */
                            0U); /* 时长未知 */
```

`playback_play_url()` 只接受以 `http://` 或 `https://` 开头的 URI。资源类型
可使用：

```text
QM_PLAYER_RESOURCE_TYPE_MP3
QM_PLAYER_RESOURCE_TYPE_WAV
QM_PLAYER_RESOURCE_TYPE_AAC
QM_PLAYER_RESOURCE_TYPE_FLAC
QM_PLAYER_RESOURCE_TYPE_PCM
```

URL 资源使用 `qm_player` 内置的 HTTP source；应用层不需要实现
`qm_player_source_if_t`。该函数成功只表示播放请求已经入队，不表示喇叭已经
开始出声。

## 播放本地文件

```c
int ret = playback_play_file("/data/song.mp3",
                             QM_PLAYER_RESOURCE_TYPE_MP3,
                             file_size,
                             duration_s);
```

本地文件需要在 `playback_init()` 时传入应用自己的 `qm_player_source_if_t`。
source 适配器只负责 `open/read/close`，缓冲、解码、PCM 输出和音频抢占仍由
`qm_player`、解码器和 `qm_audio` 负责。

## 播放控制和状态

```c
int playback_pause(void);
int playback_resume(void);
int playback_stop(void);
int playback_get_status(qm_player_status_t *status);
int playback_deinit(void);
```

`playback.c` 保存最近一次成功入队的 operation ID，并用它执行暂停、恢复和
停止。控制请求是异步入队的：

- `playback_pause()` 保留 source、解码器会话和已经缓存的 PCM；
- `playback_resume()` 恢复同一个 operation；
- `playback_stop()` 请求停止当前 operation；
- `playback_deinit()` 销毁 player，调用方仍负责管理共享 `qm_audio`；
- 没有播放器或没有当前 operation 时，暂停、恢复和停止会返回
  `-QM_ENOENT`。

播放请求使用当前实现固定的音频仲裁策略：低优先级、队列策略、暂停采集、
可靠投递，申请音频 lease 最多等待 3000 ms。播放器事件由
`playback.c` 内部记录为：

```text
event=<type> operation=<id> error=<error> position=<ms>
```

请求被接受、开始播放、暂停、恢复、自然结束、停止、被抢占和错误是不同的
阶段，不能把 `playback_play_*()` 的返回值当成播放完成。

## 查询资源元数据

当前工程编译的是 MQTT 元数据模式，调用方需要构造：

```c
playback_metadata_t metadata = {
    .did = device_did,
    .iot_handle = ljyd_cloud_get_handle(),
};
```

`did` 和 `iot_handle` 必须在每次调用时有效。元数据查询是同步调用，应该在
应用 worker 中执行，不能在 MQTT 收包回调里等待资源服务回复。

### 打印第一页

```c
int ret = playback_print_first_page(&metadata, "resource-bundle-name");
```

该函数调用 `qm_res_mgr_get_list()`，请求第 1 页、每页最多 10 条资源，并在
内部打印每条资源的 ID、标题、业务类型和 `audioType`。示例为了只读取一页，
页回调返回 `-QM_EINTR`；如果该页业务码为 0，包装函数会把这个中断转换为
`QM_EOK`，最后释放列表内存。

该接口不会把列表返回给调用方。需要完整列表或自定义分页处理时，应直接
使用 `qm_res_mgr_get_list()`，并在结束后调用：

```c
qm_res_mgr_list_free(&list);
```

### 查询分类

```c
qm_res_mgr_category_list_t categories;
int ret = playback_get_categories(&metadata, &categories);
if (ret == QM_EOK) {
    /* 使用 categories.items[0..categories.count) */
}
qm_res_mgr_category_list_free(&categories);
```

查询子分类：

```c
qm_res_mgr_child_category_list_t children;
int ret = playback_get_child_categories(&metadata,
                                        category_id,
                                        "resource-bundle-name",
                                        &children);
if (ret == QM_EOK) {
    /* 使用 children.items[0..children.count) */
}
qm_res_mgr_child_category_list_free(&children);
```

分类结果及其中的字符串由 `qm_res_mgr` 动态分配，必须使用对应的释放 API。

### 根据资源 ID 播放

```c
int ret = playback_play_id(&metadata, resource_id);
```

该函数的实际顺序是：

```text
MQTT getResourceDetail
        -> 读取 detail.url、audioType、fileSize、totalDuration
        -> 映射为 qm_player_resource_t
        -> qm_player_play()
        -> 释放 detail 返回的动态字段
```

`audioType` 当前映射为 `mp3`、`wav`、`aac`、`flac` 和 `pcm`。未知类型会映射
为 `QM_PLAYER_RESOURCE_TYPE_UNKNOWN`，通常需要由解码器或业务层拒绝。
`qm_player_play()` 会复制播放资源的字符串，因此 `playback_play_id()` 在提交
后可以安全释放 `qm_res_mgr_get_detail()` 返回的 detail。

## 直接下载资源

```c
int playback_download(const qm_res_mgr_resource_t *resource,
                      qm_res_mgr_download_data_cb_t on_data,
                      void *user,
                      qm_res_mgr_download_result_t *result);
```

该接口是同步下载，不会自动写文件，也不会自动提交给 `qm_player`。每收到一
段数据就调用一次 `on_data`：

```text
on_data(resource, data, len, offset, total_size, user)
```

应用层可以在回调里写文件、做缓存或送入自己的处理管线。回调返回
`QM_EOK` 继续，返回负的 `QM_E*` 会终止下载。下载结束后由应用检查
`qm_res_mgr_download_result_t`，并自行处理临时文件、校验和缓存策略。

资源列表和详情中的动态字符串分别由以下 API 释放：

```c
qm_res_mgr_list_free(&list);
qm_res_mgr_resource_free(&detail);
```

## 唤醒打断

当前 `apps/main/agent/agent_app.c` 的唤醒回调顺序是：

```text
唤醒词
  -> playback_stop()
  -> qm_agent_interrupt()
```

因此，资源播放器已经通过 `playback_init()` 创建并有活动 operation 时，唤醒
会先请求停止资源播放；如果同时存在云端对话，`qm_agent_interrupt()` 会清空
本地 TTS 队列并重新打开上行麦克风。

当前 Agora transport 的后端中断发送接口仍返回 `-QM_ENOSYS`。此时应用层只记录
本地打断结果，不会停止或重启 agent 会话。

## 云控边界

当前配置关闭了：

```c
#define CONFIG_QM_PLAYER_CLOUD_SUPPORT 0
```

所以本工程不会自动注册 `method=player`、`playerReply` 的 QUMI 云控处理器。
如果业务层收到 MQTT `player` 命令，应用必须自己完成：

```text
MQTT 收包
  -> 应用层解析并投递到 worker
  -> 查询资源或读取 URL
  -> playback_play_id() / playback_play_url()
  -> 应用层发送业务回复
```

不能因为 `qm_player_cloud.c` 被加入编译列表，就认为云控已经接通；真正的
开关由 `CONFIG_QM_PLAYER_CLOUD_SUPPORT` 决定。

## 生命周期和线程要求

- `playback_init()` 成功后，保持传入的 audio、decoder 和 source 接口有效；
- `playback_play_*()`、暂停、恢复和停止只提交播放器命令，实际状态通过事件
  或 `playback_get_status()` 观察；
- `qm_res_mgr` 的列表、分类、详情和下载接口都是同步接口，应在应用 worker
  中执行；
- 列表、分类和详情返回的动态内存必须使用 QUMI 提供的释放函数；
- 先停止并销毁 `playback`，再销毁它依赖的共享 `qm_audio`；
- 不要让多个任务并发切换资源环境；当前 MQTT 模式本身不支持运行时环境切换。

## 常见返回值和日志

| 返回值 | 含义 |
| --- | --- |
| `QM_EOK` | 请求已接受或同步查询成功 |
| `-QM_EINVAL` | 参数为空、资源 ID 无效、URL 格式错误或元数据不完整 |
| `-QM_EINIT` | 播放器尚未通过 `playback_init()` 创建 |
| `-QM_ENOENT` | 没有可暂停、恢复或停止的当前 operation |
| `-QM_ENOSYS` | 当前构建不支持该功能，例如 MQTT 模式调用资源环境切换 |
| `-QM_EIO` | 资源服务业务失败或下载/解码链路错误 |

排查时应先区分：

1. 是否调用过 `playback_init()`，以及 decoder/source 是否真实有效；
2. `playback_play_*()` 是否只是入队成功，后续是否有播放器事件；
3. 元数据查询是否使用了有效的 DID 和 `ljyd_cloud_get_handle()`；
4. 资源 URL 是否可访问、`audioType` 是否有对应解码器；
5. 日志是否来自 `qm_player` 资源链路，还是来自 `qm_agent` 的语音下行链路。
