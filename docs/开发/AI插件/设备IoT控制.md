# 设备IoT控制

> 来源：[灵机官方语雀 · 设备IoT控制](https://xjiangiot.yuque.com/afahpl/qrhct7/dc7i3ia268k44vl1)。原文内容更新时间：2026-10-28（UTC 日期）；整理日期：2026-10-29。
>
> 本文整理源页面的文字、表格、代码和可解析的流程图，保留官方示例口径；不代表本项目已采用或已验证。图示不在本地保存，未转录的图中文字、引脚位置和接线方向需回源核对。附件已有本地整理时链接到本地，其余保留源站链接，未下载、执行或展开。

## 一、介绍

### 功能

你在公司点一下手机，家里的灯就亮了，这件事隔着三样东西：手机、云平台、灯。

**一次开灯实际怎么走：**

1. 你在手机小程序里把某个设备的开关拨到开。
2. 小程序把这条属性发给云，云平台认出是你操控的是你家的哪一台设备。
3. 云平台把指令下发给设备，设备把灯点亮。

设备IoT控制，就是用一份大家都能看懂的功能表，让云平台在手机和设备之间传命令、同步状态。灯亮不亮**由设备自己执行**；云平台负责认人、认设备、记住并转发。

灵机一动就是拥有这种能力的一个云平台，而且我们在此基础上更智能，用户可以通过语音对话的方式触发IoT控制，只要设备在你手上，你就可以随时控制它，不再需要打开手机操作小程序。

### 框图

以下是基于语音链路的设备IoT控制时序图：

```text
@startuml
title 设备IoT控制时序图

actor 用户
participant "设备" as Device
participant "灵机云平台" as Cloud

用户 -> Device : 语音输入
Device -> Cloud: 上报麦克风数据
Cloud -> Cloud : 音频处理、意图识别
Cloud -> Device : IoT指令下发
Device -> Device : 根据物模型解析指令，\n执行外设控制

@enduml
```

## 二、使用说明

对于使用灵机平台的开发者来说，要做的只有在收到IoT指令后，**解析指令并执行自己的软件逻辑**。云端连接、报文接收和物模型解包等均已在SDK的`qm_iot`组件内部实现，参考工程已经接好了这套组件，并给出了一个控制例程。基本实现、验证方式如下：

### 使用前准备

接入本功能前，确认智能体中已添加以下插件，具体接入方式见第三节：

〔图示见来源页面；未转录图中文字〕

### 基本实现

云平台每下发一个IoT指令，设备都能收到三个值：`siid`、`piid`和属性值。参考工程用一张表把「`siid` + `piid`」对应到处理函数。**新增一种控制，就是加一行表项，并实现这个处理函数**。

对应表项形如：

```c
{SIID_VOLUME, PIID_VOLUME, 		on_volume_set},			// siid, piid, 处理函数
{SIID_VOLUME, PIID_VOLUME_ADJ,	on_volume_adjust},
```

`siid`、`piid` 的取值和灵机平台上对应产品的物模型一致。处理函数从指令里取出值，再去控制外设：

```c
int on_volume_set(qm_spec_property_t *property)
{
    int volume = 0;

    // 取值
    if (property == NULL ||
        qm_spec_property_unpack_number(property, &volume) != QM_EOK) {
        return -QM_EINVAL;
    }

    /* 操作外设，设置喇叭音量 */

    return QM_EOK;
}
```

数值用 `qm_spec_property_unpack_number()`，布尔用 `qm_spec_property_unpack_bool()`，字符串用 `qm_spec_property_unpack_string_direct()`。函数返回前把外设设到目标状态即可。需要掉电记住、或让平台看到最新状态时，再自行保存并调用 `qm_iot_report()` 回写。

### 验证方式

按照上述方式接入后，通过语音对话尝试触发IoT控制，观察设备日志确认是否收到指令，并查看外设是否按照预期发生变化。以参考工程内已集成的音量控制举例：

1. 确认设备在语音对话状态中，说“音量调到最小”。设备应有如下日志：

```c
[022273]<I> asr final=0 text=音量调到最小
[024081]<I> asr final=1 text=音量调到最小。
[024081]<I> dialog=thinking
[025180]<I> dialog=speaking
```

2. 查看设备日志，确认是否收到IoT指令。设备应有如下日志：

```c
[014702]<D> PUBLISH
[014702]<D>         Packet Ident : 00000000
[014702]<D>         Topic Length : 35
[014702]<D>           Topic Name : $aws/things/100000647/shadow/update
[014707]<D>     Payload Len/Room : 64 / 8105
[014711]<D>       Receive Buflen : 8144
[014714]<D> delivering msg ...
[014717]<D> topic be matched
[014720]<I> iot recv set(0) did=0 siid=4 piid=2 code=0 value=i32 4
```

3. 观察设备外设，确认是否按预期产生变化（喇叭音量变小）。

如果上述三步均通过验证，那么代表这个设备控制是接入成功的。

如果上述三步没有通过验证，可以按以下思路排查问题：

1. 如果第1步失败，查看设备日志，看打印的文本是否和说话内容相符。
2. 如果第2步失败，即没收到IoT指令，那可能是云平台没有识别出意图，尝试换个更明确的控制语料。
3. 如果第3步失败，那可能是这对 `siid`/`piid` 还没有正确登记到表里。检查一下表内的`siid`和`piid`是否和物模型的相符，平台上的物模型值必须和代码里的宏一致。

具体实现见参考工程的 `apps/main/lj_components/iot_control` 目录。

## 三、平台接入说明

### 选择产品

〔图示见来源页面；未转录图中文字〕

### 添加物模型

〔图示见来源页面；未转录图中文字〕〔图示见来源页面；未转录图中文字〕

### 新建场景

〔图示见来源页面；未转录图中文字〕

### 编辑场景

〔图示见来源页面；未转录图中文字〕

### 观察设备日志

```c
[101869]<I> asr final=1 text=我要睡觉了。
[101870]<I> dialog=thinking
[102536]<I> dialog=speaking
[102812]<I> tts final=0 text=好的，已为你把灯光调暗
[102915]<D> PUBLISH
[102915]<D>         Packet Ident : 00000000
[102915]<D>         Topic Length : 35
[102915]<D>           Topic Name : $aws/things/100000754/shadow/update
[102919]<D>     Payload Len/Room : 65 / 8105
[102924]<D>       Receive Buflen : 8144
[102927]<D> delivering msg ...
[102930]<D> topic be matched
[102933]<I> agent text final=1 text=好的，已为你把灯光调暗
[102939]<D> set_recv[0] payload_len=65
[102943]<D> recv[100000754] value_len=29
[102946]<I> iot recv set(0) did=0 siid=8 piid=2 code=0 value=i32 10
```

### 视频教程

[设备iot控制-平台配置.mp4（源站附件）](https://xjiangiot.yuque.com/attachments/yuque/0/2026/mp4/38612929/1793166024843-c3839cda-5501-459e-9e2c-3de79141f015.mp4)
[设备iot控制-设备端.mp4（源站附件）](https://xjiangiot.yuque.com/attachments/yuque/0/2026/mp4/38612929/1793166033435-0fa09af6-8ad3-4416-8f0c-a6d5852a9321.mp4)

---

相关文档：[设备开发资料索引](../灵机设备开发资料索引.md) · [项目文档索引](../../README.md)。
