# 应用层闹钟

这个目录位于 `apps/main/lj_components/alarm`，是平台 `qm_alarm` 组件之上的一层应用
包装，不是通用闹钟组件。它提供：

- 初始化/逆初始化：创建并启动 `qm_alarm`，退出时停止并销毁；
- 事件搬运：把组件的 4 类通知投递到默认事件循环，在它的任务上下文里执行行为回调；
- 条目操作：新增、修改、删除、启用/禁用；
- 响铃操作：关闭当前响铃、稍后提醒、取消稍后提醒；
- 查询：下一个闹钟、整份列表、当前运行态；
- 调试命令行 `alarm ...`（仅在 `CONFIG_QM_CLI_SUPPORT` 打开时编译）。

`app_alarm_init()` 只需要一个 `qm_iot` 会话句柄；行为回调在模块内部登记，上层不必
再注册。示例行为是"响铃时播一遍已内嵌的开机音效，正在对话时把喇叭从对端 TTS 手里
拿过来，播完对话还能继续出声"，产品化时替换 `app_alarm.c` 里的
`app_alarm_behavior()` 即可。

`apps/main/CMakeLists.txt` 已把 `lj_components/alarm/app_alarm.c` 加入编译并导出头
文件路径；`apps/main/main.c` 在 `agent_app_init()` 之后调用 `app_alarm_init()`。

## 示例产品标识

本工程当前连的是灵机测试服，产品身份定义在 `apps/main/cloud/ljyd_cloud.h`（闹钟的
云端同步、下发都走这个 `qm_iot` 会话）：

```c
#define CONFIG_XJIANG_CLOUD_PID    11665
#define CONFIG_XJIANG_CLOUD_SECERT "F7B6033204064B698FEDE033605DDDBA"
```

这是本仓库本地示例用的测试产品，换成正式产品时改这一处即可；云端闹钟下行
`alarm/update` 的 topic 会随之变成 `/<PID>/<DID>/device/down`。

## 依赖边界

本模块唯一的业务依赖是平台闹钟组件 `qm_alarm`；投递事件用的 `qm_work` 是它的传递
依赖（`qm_alarm` 的 Kconfig 本身就 `depends on QM_WORK_SUPPORT`），没有引入新依赖。
墙钟时间由组件内部通过 `qm_time` 计算，应用层不需要自己维护 timestamp。

需要 `apps/main/qm_config/qm_config.h` 打开：

```c
#define CONFIG_QM_ALARM_SUPPORT 1
#define CONFIG_QM_ALARM_MAX_ALARMS 8
#define CONFIG_QM_ALARM_SNOOZE_INTERVAL_MIN 5
```

### 总开关

`CONFIG_QM_ALARM_SUPPORT` 同时管平台组件和应用层模块，置 0 时**整个闹钟功能都不进
构建**，不只是"不注册"：

| 位置 | 置 0 后的行为 |
| --- | --- |
| `apps/main/CMakeLists.txt` | 条件添加，`lj_components/alarm/app_alarm.c` 和 `qm_alarm/src/qm_alarm.c` 都不编译 |
| `apps/main/main.c` | `app_alarm_init()` 调用被 `#if` 包住，不会注册组件（CLI 命令组注册在 `app_alarm_init()` 内部，因此也一起关掉） |
| `apps/main/cloud/time_sync.c` | 对时后不再通知闹钟重排 |

只把注册包起来是不够的：`qm_alarm.h` 没有用宏包住（类型和声明一直在），而
`qm_alarm.c` 的实现整体在 `#if defined(CONFIG_QM_ALARM_SUPPORT) && CONFIG_QM_ALARM_SUPPORT`
里，宏关掉时它编成空 TU、没有任何符号。这时如果 `app_alarm.c` 还参与编译，就会
链接不到 `qm_alarm_*` 而报 undefined reference，所以调用点和源文件必须一起关。

`qm_alarm` 默认不编译，`apps/main/CMakeLists.txt` 里 `qm_alarm/src/qm_alarm.c` 是手工
列进源文件列表的，没有这两个开关就不会生效。

## 触发链路

```text
CLI / 云端下行 / 本地调用
        |
        v
   app_alarm_*()            薄转发，不做业务判断
        |
        v
   qm_alarm 组件            owner task 内排程、到点触发
        |
        v
   alarm_notify_cb          只拷贝事件快照 + 投递，立即返回
        |
        v
   qm_post_delayed_action(ms=0)
        |
        v
   alarm_evt_work           事件循环（qm_work 默认 loop）任务上下文
        |
        v
   app_alarm_behavior       示例：起提示音（对话中由 agent 让出喇叭）
```

组件通知回调跑在组件的 owner task 上，里面只做值拷贝和投递，不调本模块接口。真正的
业务动作在 `alarm_evt_work` 里跑，所以行为回调里可以安全地调
`app_alarm_close_ringing()`、`app_alarm_snooze()` 或切页面。

### 行为回调里不要做的事

`alarm_evt_work` 跑在 **全局默认事件循环任务** 上，它同时承载 `qm_agent` 的
reconcile work、`qm_alarm` 的排程 work 等，所以里面不能有任何阻塞调用——冻住的不只是
闹钟，而是整个事件循环（表现为整机无日志、唤不醒、CLI 也停）。

**不要在行为回调里调 `agent_app_leave()`（`qm_agent_stop()`）**，它会在事件循环上做
agora 拆链，实测"正在对话时到点响铃"会卡死。

让对话让出喇叭由 `boot_sound` 在自己的任务里做，顺序是：

```text
agent_app_set_resource_mode(QM_TRUE)    预留通路：暂停采集、关上行
agent_app_abort_output()                停对端输出，并重开 TTS 接收
agent_app_play_local_audio()            播提示音
agent_app_set_resource_mode(QM_FALSE)   释放通路：恢复采集与上行
```

不要用自己的播放会话用 `INTERRUPT_ALL` 去抢 agent 的 TRANSPORT 会话：agent 的
`on_preempt` 会关掉 `tts_accepting`，而 agora 上传不回来能重新打开它的事件，之后 TTS
数据全被丢弃、喇叭没声。

## 初始化与逆初始化

```c
qm_err_t app_alarm_init(void *iot_handle);
void app_alarm_deinit(void);
qm_bool app_alarm_is_ready(void);
```

- `iot_handle` 传 `ljyd_cloud_get_handle()` 时开启云端同步；传 `NULL` 时只做本地闹钟；
- 单例：重复调用 `app_alarm_init()` 直接返回 `QM_EOK`；
- 初始化与逆初始化都不动已有闹钟数据，不清空列表、不删 KV；
- `app_alarm_deinit()` 会停止调度、注销云端方法、销毁实例并清空行为回调。
  目前启动流程不调用它，仅作为接口保留。

## 事件与行为回调

`app_alarm_event_t` 是一次通知的值语义快照，回调返回后仍然有效，可以直接保存：

| 字段 | 说明 |
| --- | --- |
| `notify` | `QM_ALARM_NOTIFY_RING` / `DISMISS` / `SNOOZE` / `CHANGED` |
| `alarm_id` | 关联闹钟 id，0 表示无 |
| `source` | `device` / `app`，与 `alarm_id` 组成主键 |
| `hour` / `minute` / `repeat_mask` | 触发时刻与重复掩码 |
| `enabled` / `vibrate` / `tone` | 使能、振动、铃声选择（`tone` 是值类型，可整结构体拷贝） |

`extensions` 不在快照里：它是组件内的借用指针，只在通知回调期间有效。需要完整条目
时用 `app_alarm_get_list()` / `app_alarm_get_state()` 再查。

回调运行在 workqueue 任务里，可以安全调用本模块接口，但不要在里面对长时间阻塞。

## 条目操作

```c
void       app_alarm_item_defaults(qm_alarm_item_t *item);
qm_err_t   app_alarm_add(const qm_alarm_item_t *item);
qm_err_t   app_alarm_modify(const qm_alarm_item_t *item);
qm_err_t   app_alarm_delete(const char *source, uint8_t alarm_id);
qm_err_t   app_alarm_enable(const char *source, uint8_t alarm_id, qm_bool enabled);
```

`app_alarm_item_defaults()` 填一份可直接新增的条目：`source=device`、`alarm_id=AUTO`、
`enabled`、`tone.type=LOCAL`、`snooze` 关闭、`extensions=NULL`。`alarm_id` 用
`QM_ALARM_LOCAL_ID_AUTO` 时由组件分配第一个空闲 id。

`app_alarm_modify()` 要求 `item` 是完整快照并且带有既有的 `(source, alarm_id)`；
`app_alarm_add()` 在 `source` 为空时按 `device` 补齐。

## 响铃流程

```c
qm_err_t app_alarm_close_ringing(void);   /* 关闭当前正在响铃的闹钟 */
qm_err_t app_alarm_snooze(void);          /* 进入稍后提醒，受该闹钟自身 snooze 配置限制 */
qm_err_t app_alarm_cancel_snooze(void);   /* 取消待响的稍后提醒，回到空闲 */
```

一次性闹钟响过之后由组件自动置为非使能（`enabled=0`），应用层不需要自己判断闹钟
类型。

## 查询接口

```c
qm_err_t app_alarm_get_next(qm_alarm_item_t *out, qm_time_t *trigger_time);
qm_err_t app_alarm_get_list(qm_alarm_item_t *out, uint16_t capacity, uint16_t *count);
qm_err_t app_alarm_get_state(qm_alarm_state_t *state, uint8_t *alarm_id);
```

这三个接口是给界面渲染和兜底排查用的，不是触发主路径。`qm_alarm_get_state()` 在组件
内部是异步投递，调用后需要给组件一点时间才拿得到最新状态。

## 对时与云端同步

```c
qm_err_t app_alarm_time_changed(void);   /* NTP/手动对时之后必须调用 */
qm_err_t app_alarm_report(void);         /* 主动整表上报，一般不需要 */
```

组件按内部时钟计算下一次触发，**对时之后不调用 `app_alarm_time_changed()` 会按旧时钟
误触发**。当前工程的调用点是 `apps/main/cloud/time_sync.c`：`ntpdate` 对时成功后写
RTC，再通知本模块重新排程（`time_sync_start()` 由 `apps/main/cloud/ljyd_cloud.c` 在
网络可用时发起）。

云端一整表上报、断网延迟补报、下行指令解析都由 `qm_alarm` 内建，应用层不需要写云同步
代码；下行会解析 `alarm/update` 的 `create/modify/delete/enable/update/close/snooze`
等子方法。设备侧只做透传与行为响应，语音"设个闹钟"这条链路的意图转换在云端完成。

## 调试命令行

CLI 由内核 `qm_init()` 自动初始化，命令注册在 `app_alarm_init()` 内部，输入输出走
`CONFIG_QM_LOG_UART` 绑定的那路 UART。

```text
alarm list              列出全部闹钟
alarm next              下一个触发的闹钟与绝对时间
alarm state             当前运行态（idle/ringing/snoozing）与关联 id
alarm add +N            N 分钟后（一次性）
alarm add HH:MM         当天 HH:MM（一次性）
alarm del <id>          删除
alarm on|off <id>       使能/非使能
alarm close             关闭当前响铃
alarm snooze            稍后提醒
alarm cancel            取消稍后提醒
alarm clear             逐条清空全部闹钟
alarm test              直接播一遍提示音，用于单独验证播放通路
```

组件没有 clear-all 接口，`alarm clear` 只能按 `(source, id)` 主键逐条删。

## 返回值与排查

| 返回值 | 含义 |
| --- | --- |
| `QM_EOK` | 请求已接受 |
| `-QM_EINVAL` | 参数为空或字段非法 |
| `-QM_EINIT` | 尚未调用 `app_alarm_init()` |
| `-QM_ENOMEM` | 创建组件实例失败 |
| `-QM_ERROR` | 启动组件失败 |

排查顺序：

1. 先确认 `CONFIG_QM_ALARM_SUPPORT` 已打开且 `qm_alarm.c` 在编译列表里；
2. `alarm state` 是否能在到点前后从 `idle` 变成 `ringing`；
3. `alarm list` 里的时刻是否是真实时间——不对说明 `app_alarm_time_changed()` 没被调到；
4. 有 `RING` 日志但没声音，看 `boot_snd` 相关日志：当前提示音是复用内嵌的开机音效，
   走的是 `qm_audio` 播放仲裁，被对话占用时会申请失败重试；
5. 云端不下发 `alarm/create` 通常是云端意图转换问题，不是设备侧协议问题。
