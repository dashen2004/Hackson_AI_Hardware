# Ticko 番茄钟按钮接入要点

来源：[灵机一动《番茄时钟》](https://xjiangiot.yuque.com/afahpl/qrhct7/acnxplbrn9lxfw3l)第三节“谁先进、谁先出”与“设备侧必须上报的时机”；本项目归档 `docs/开发/AI插件/番茄时钟.md`。先核对线上文档与当前固件／平台版本，不能将归档文本当作实时实测。

## 原生插件的两个方向

| 发起端 | 平台／设备通道 | 正确行为 |
| --- | --- | --- |
| 语音，经平台发起 | `device/down`，`method=pomodoro`、`subMethod=start/stop`、`params.source=cloud` | 设备执行后通过 `pomodoroReply` 回原 `id` 与 `subMethod`，成功 `code=0`、失败 `code=1`。 |
| 设备本地操作发起（含面板属性触发） | 本地计时动作，随后由设备通过 `device/up` 上报 `method=pomodoro`、实际`subMethod`、`params.source=local` | `start`请求进入学习模式，`pause`／`resume`保持学习态，`stop`请求退出；自然结束也要上报`stop`。是否生效须验平台。 |
| 本地上报经云端回环 | `device/down` 的 `source=local` | 只回成功，不重复执行动作或再次上报。 |

官方示例 `{"method":"pomodoro","subMethod":"start","id":1301,"params":{"source":"cloud"}}` 描述的是**云端下行**。`id=1301` 只是示例关联号，不能由按钮固定复用；本地上行需由 SDK／应用生成本次消息 ID。SDK 的 `qm_iot_down_reply_ex` 接收原始 method `pomodoro`，内部追加 `Reply`；`qm_iot_up_report_ex` 发送设备上行。其返回 0 表示发布请求已发起，不代表平台已确认状态。

平台在原生番茄学习模式会拦截闲聊。暂停／继续不退出学习模式；本地停止、自然完成后必须让平台收到 `stop`，再用一次闲聊验证拦截已解除。本地启动后用闲聊验证已进入模式。对组件事件只排队网络同步，避免在计时回调里阻塞发布。

## Ticko 当前代码与产品配置的分界

- `src/firmware/apps/main/lj_components/pomodoro/pomodoro.c` 注册 `pomodoro` 下行、执行并回执，负责重连状态同步与本地上报；`qm_iot_api.h` 定义实际 SDK 上下行 API。
- `pomodoro_start_for(seconds)`、`pomodoro_execute(action)` 和物模型 `12.1 FocusDuration` 是 Ticko 自定义专注路径。这条路径与原生插件禁闲聊态分离；不要通过全局修改本地计时函数，让所有 12.x 或 CLI 计时都进入原生插件模式。
- H5 按钮的当前接线使用写入型 `12.5 PluginStart`：面板经 `xj.device.set` 传明确秒数，固件成功启动唯一计时实例后选择原生插件学习态，并由设备上报 `pomodoro/start`、`source=local`。结束／暂停／继续写 `12.2 FocusAction` 的1／2／3；插件计时暂停／继续分别上报`pause`／`resume`，停止和自然完成才上报`stop`。普通12.1自定义计时不冒充插件事件。同一连接不因暂停／继续重复上报`start`；重连时按实际状态重新宣告，暂停态先`start`再`pause`。属性状态与插件事件分别重试失败的一路。面板以设备 `12.1` 实际时长、`12.3` 阶段和 `12.4` 剩余时间回报核对动作；12.2／12.5自身只写，不存在同值读回。PID11878物模型版本3已新增12.5且服务列表审核通过；旧归档仅含12.1～12.4，设备实际关联和真机效果仍待验证，详见 `docs/开发/Ticko自定义专注计时与呼吸灯.md`。
- 面板只从宿主原始远端事件或明确GET回包接受状态，不用 SDK 乐观缓存或发送Promise确认设备执行。App GET按查询时DID／PID、连接代、键集与时间归属；发命令或切设备后使旧查询失效。Weapp的`getAttrs_reply`缺请求关联号，即使带DID也只更新显示，不能确认按钮成功；缺DID时还须首次设备绑定且有未过期明确查询。只有带当前正确DID的主动`onDeviceDataChange`可确认Weapp按钮动作；切设备后的无DID回包和主动无身份推送均拒绝。宿主若不能提供可验证身份或请求关联，报告“待核对”，并通过设备日志／平台状态另验，勿声称H5已确认插件云态。
- 当前已发布 Ticko V2 只挂“设备控制”插件；未发布 V3 草稿已挂载“番茄时钟”并在重新进入时可见，但该草稿未发布、未设为产品默认。若 H5 按钮经 12.x 控制设备，首先验收设备计时与属性状态；只有确实通过受支持的原生插件控制与同步链路，才验收原生学习模式。
- 原生下行示例没有时长；本项目云端裸 `start` 目前会因缺少显式 `focus_sec` 拒绝，防止把用户指定时长替换为开发默认值。若要通过面板启动任意时长，应选能传秒数并有合法鉴权的接口；不能仅复制示例报文。

可复现资料：`docs/开发/Ticko自定义专注计时与呼吸灯.md`、`docs/开发/Ticko番茄时钟最小接入与验证.md`、`src/firmware/apps/main/lj_components/pomodoro/README.md`。验证时对照当前源码和平台发布版本；这些资料中的历史秒级联调不代表当前端到端按钮已通过。
