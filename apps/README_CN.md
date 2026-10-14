# apps 说明

应用入口在 `main/`。底层能力来自 `qumios-dev`（qm_iot / qm_agent / qm_audio / VB701N 驱动等），本目录只做板级装配和业务编排。

## 目录

```
main/
├── main.c              # 启动顺序：外设 → 语音 → 云 → 配网 → agent
├── qm_config/          # 产品与功能配置（pid、栈大小、VOICE_BACKEND 等）
├── peripheral/         # 板级外设
│   ├── bsp_uart.*      # 语音芯片串口、vb701n_bind
│   └── bsp_io.*        # 流控脚、长按复位等 GPIO
├── voice/              # 语音会话（qm_audio 创建、唤醒回调；换芯片主要换驱动句柄）
├── cloud/
│   ├── ljyd_cloud.*    # IoT 初始化、动态注册、连云事件
│   └── ljyd_provision.*# BLE 配网 / 已保存 WiFi 上电联网
└── agent/
    ├── agent_app.*     # 创建 qm_agent，云连上 + 唤醒词后才 start
    └── private/        # 二开一般不用改：PCM↔Opus、pcm_driver 包装层
```



## 软件架构（简要）

```
                    ┌─────────────┐
                    │   main.c    │
                    └──────┬──────┘
           ┌───────────────┼───────────────┐
           ▼               ▼               ▼
     peripheral         voice           cloud
      uart/io         qm_audio      ljyd_cloud
           │               │        ljyd_provision
           │               │               │
           │               ▼               ▼
           │          唤醒词事件      CLOUD_CONN
           │               │               │
           └───────────────┴───────┬───────┘
                                   ▼
                              agent_app
                           (qm_agent create
                            后，双条件 start)
                                   │
                    ┌──────────────┴──────────────┐
                    ▼                             ▼
              private/pcm_drv               Agora 传输
              (芯片 PCM 时编解码)            (qm_agent)
                    ▼
              VB701N 驱动 (qumios-dev)
```

要点：

1. **初始化顺序**：`bsp_uart` → `bsp_io` → `voice` → `ljyd_cloud` → `ljyd_provision` → `agent_app`。`agent` 创建需要 `iot_handle`，且应在 TLS 连云之前完成（避免内部堆碎片导致音频任务栈分配失败）。
2. **连云与配网分开**：配网负责 WiFi；拿到 IP 后通知 cloud 做 dynreg / MQTT。`CLOUD_CONN` / `DISCONN` 通过回调交给 agent。
3. **agent 何时 start**：`qm_agent_create` 在 `agent_app_init` 里完成；`qm_agent_start` 仅在「已 CLOUD_CONN」且「语音芯片上报唤醒词」时调用。断云会 stop 并清状态。
4. **音频路径**：上报给 agent 的格式固定为 Opus 16 kHz / 20 ms。芯片侧若是 PCM，由 `agent/private` 做编解码包装；若芯片本身出 Opus，则直接挂 VB701N 驱动。

