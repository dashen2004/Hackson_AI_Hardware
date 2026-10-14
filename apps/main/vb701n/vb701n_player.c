#include "qm.h"
#include "qm_log.h"
#include "qm_uart.h"
#include "qm_gpio.h"
#include "qm_work.h"
#include "vb701n.h"
#include "vb701n_player.h"
#include "driver/gpio.h"

#include <string.h>

#define LOG_TAG "vb701n_player"

/* 1: 双向 PCM。0: 双向 Opus 40 字节/20 ms */
#ifndef VB701N_APP_PCM
#define VB701N_APP_PCM  1
#endif

#if CONFIG_VOICE_BACKEND_AGORA && VB701N_APP_PCM
#include "agora_pcm_bridge.h"
#endif

/* 1: 麦回环。0: 播 xj_test_20ms。只在 VB701N_APP_DEMO_PLAY 打开时生效 */
#ifndef VB701N_APP_LOOPBACK
#define VB701N_APP_LOOPBACK  0
#endif

/* 1: 芯片起来后播测试音频。0: 开麦，交给上层 */
#ifndef VB701N_APP_DEMO_PLAY
#define VB701N_APP_DEMO_PLAY  0
#endif

/* 1: 关掉发送间隔并循环播，只靠流控脚停发。0: 按帧间隔播一遍 */
#ifndef VB701N_APP_FLOW_STRESS
#define VB701N_APP_FLOW_STRESS  0
#endif

#if VB701N_APP_DEMO_PLAY && !VB701N_APP_LOOPBACK
extern const uint8_t xj_test_20ms[3160];
#define VB701N_PLAY_FRAME_LEN   (VB701N_APP_PCM ? 320 : 40)
#define VB701N_PLAY_BURST       2
#endif

/* 帧格式在 vb701n_audio_fmt.c。 */
extern const qm_audio_format_t g_in_fmt;
extern const qm_audio_format_t g_out_fmt;

#if VB701N_APP_PCM
#define VB701N_LOOP_FRAME_MAX        512
#define BOARD_VB701N_TX_INTERVAL_MS  5
#else
#define VB701N_LOOP_FRAME_MAX        160
#define BOARD_VB701N_TX_INTERVAL_MS  20
#endif

/*
 * 这块 ESP32-S3 板上语音芯片原来接在 GX8006 那组脚。
 * VB701N 波特率按参考工程用 2M。流控脚 GPIO38，高电平停发。
 */
#define BOARD_VB701N_UART_PORT       1
#define BOARD_VB701N_UART_TX_PIN     40
#define BOARD_VB701N_UART_RX_PIN     39
#define BOARD_VB701N_UART_BAUD       2000000
#define BOARD_VB701N_FLOW_PIN        38

static qm_uart_dev_t g_board_uart = {
    .config = {
        .baud_rate = BOARD_VB701N_UART_BAUD,
        .data_bits = QM_UART_DATA_8_BITS,
        .parity    = QM_UART_PARITY_NONE,
        .stop_bits = QM_UART_STOP_BITS_1,
        .flow_ctrl = QM_UART_FLOW_CTRL_NONE,
        .pins.tx_pin = BOARD_VB701N_UART_TX_PIN,
        .pins.rx_pin = BOARD_VB701N_UART_RX_PIN,
    },
    .port = BOARD_VB701N_UART_PORT,
};

static qm_gpio_dev_t g_flow_gpio = {
    .port = BOARD_VB701N_FLOW_PIN,
    .config = {
        .mode = QM_GPIO_MODE_INPUT,
        .pull_en = QM_GPIO_PULLDOWN_ONLY,
    },
};

static int board_tx_is_busy(void)
{
    int level = qm_gpio_get_level(&g_flow_gpio);
    static uint32_t last_log;

    if (level != 0) {
        uint32_t now = qm_now_ms();
        if ((now - last_log) > 1000) {
            last_log = now;
            QM_LOGD(LOG_TAG, "tx gate busy, flow pin %u level %d",
                    (unsigned)BOARD_VB701N_FLOW_PIN, level);
        }
    }
    return level != 0;
}

static qm_audio_handle_t g_audio;
static qm_work_t g_ver_work;
static qm_work_t g_play_work;
#if VB701N_APP_DEMO_PLAY
static qm_task_t g_play_task;
#endif
static uint8_t g_ver_retry;
static uint8_t g_got_version;
static uint8_t g_play_started;

#if VB701N_APP_DEMO_PLAY
static void play_task(void *arg)
{
    qm_audio_playback_session_t session = NULL;
    qm_audio_playback_request_t request;
    qm_task_t self;
    int ret;

    (void)arg;
    memset(&request, 0, sizeof(request));
    request.priority = QM_AUDIO_PLAYBACK_PRIORITY_NORMAL;
    request.policy = QM_AUDIO_PLAYBACK_POLICY_QUEUE;
    request.acquire_timeout_ms = 3000;
#if VB701N_APP_LOOPBACK
    request.capture_policy = QM_AUDIO_CAPTURE_POLICY_KEEP;
#else
    request.capture_policy = QM_AUDIO_CAPTURE_POLICY_AUTO;
#endif

    ret = qm_audio_play_request(g_audio, NULL, &request, &session);
    if (ret != QM_EOK || session == NULL) {
        QM_LOGD(LOG_TAG, "play request fail %d", ret);
        self = g_play_task;
        qm_task_exit(&self);
        return;
    }

#if VB701N_APP_LOOPBACK
    {
        qm_audio_capture_sub_t sub;
        uint32_t frames = 0;

        sub = qm_audio_capture_sub(g_audio, 4, QM_TRUE);
        if (sub == NULL) {
            QM_LOGD(LOG_TAG, "capture sub fail");
            qm_audio_play_release(g_audio, session);
            self = g_play_task;
            qm_task_exit(&self);
            return;
        }
        QM_LOGD(LOG_TAG, "loopback running, %s", VB701N_APP_PCM ? "pcm" : "opus");

        while (1) {
            uint8_t frame[VB701N_LOOP_FRAME_MAX];
            int n;

            n = qm_audio_capture_recv(sub, frame, sizeof(frame), 200);
            if (n <= 0) {
                continue;
            }
            ret = qm_audio_play_write(g_audio, session, frame, (uint32_t)n, 1000);
            if (ret <= 0) {
                QM_LOGD(LOG_TAG, "loop write fail %d", ret);
                continue;
            }
            frames++;
            if (frames == 1 || (frames % 50) == 0) {
                QM_LOGD(LOG_TAG, "loopback frames=%u bytes=%d", (unsigned)frames, n);
            }
        }
    }
#else
    {
        uint32_t loops = 0;

        QM_LOGD(LOG_TAG, "play clip, %s, %u bytes, flow stress %d",
                VB701N_APP_PCM ? "pcm" : "opus", (unsigned)sizeof(xj_test_20ms),
                VB701N_APP_FLOW_STRESS);
        do {
            uint32_t off = 0;

            while (off < sizeof(xj_test_20ms)) {
                uint32_t n = VB701N_PLAY_FRAME_LEN * VB701N_PLAY_BURST;

                if ((sizeof(xj_test_20ms) - off) < n) {
                    n = sizeof(xj_test_20ms) - off;
                }
                ret = qm_audio_play_write(g_audio, session, &xj_test_20ms[off], n, 1000);
                if (ret <= 0) {
                    QM_LOGD(LOG_TAG, "play write fail %d off=%u", ret, (unsigned)off);
                    break;
                }
                off += (uint32_t)ret;
            }
            loops++;
            QM_LOGD(LOG_TAG, "play lap %u, %u bytes", (unsigned)loops, (unsigned)off);
            if (ret <= 0) {
                break;
            }
        } while (VB701N_APP_FLOW_STRESS);
        qm_audio_play_release(g_audio, session);
        QM_LOGD(LOG_TAG, "play finished");
    }
#endif
    self = g_play_task;
    qm_task_exit(&self);
}
#endif

static void on_chip_ready(void *arg)
{
    uint32_t vol = 20;

    (void)arg;
    if (g_play_started) {
        return;
    }
    if (g_audio == NULL) {
        return;
    }
    g_play_started = 1;
    qm_audio_control(g_audio, QM_AUDIO_CTRL_SET_VOLUME, &vol);
#if VB701N_APP_DEMO_PLAY
    if (qm_task_new(&g_play_task, "vb701n_play", play_task, NULL,
                    8192, CONFIG_QM_APP_TASK_PRIO) != QM_EOK) {
        QM_LOGD(LOG_TAG, "play task create fail");
        g_play_started = 0;
    }
#else
    {
        qm_audio_capture_policy_t policy = QM_AUDIO_CAPTURE_POLICY_KEEP;

        /* KEEP 发 02 31 01 开麦。PAUSE 会发 02 01，喇叭和麦一起停。 */
        qm_audio_control(g_audio, QM_AUDIO_CTRL_SET_CAPTURE_POLICY, &policy);
        QM_LOGD(LOG_TAG, "mic record start, opus 16k/20ms");
    }
#endif
}

static void on_audio_event(const qm_audio_event_t *event, void *user)
{
    const uint8_t *data;
    uint32_t len;

    (void)user;
    if (event == NULL) {
        return;
    }
    data = (const uint8_t *)event->data;
    len = event->data_len;
    if (event->id == QM_AUDIO_EVENT_VERSION) {
        g_got_version = 1;
        if (data != NULL && len > 0) {
            char ver[64];
            uint32_t n = len;
            if (n >= sizeof(ver)) {
                n = sizeof(ver) - 1;
            }
            memcpy(ver, data, n);
            ver[n] = '\0';
            QM_LOGD(LOG_TAG, "version: %s", ver);
        }
        qm_post_delayed_action(&g_play_work, on_chip_ready, NULL, 500);
        return;
    }
    if (event->id == QM_AUDIO_EVENT_WAKEUP) {
        char text[128];
        uint32_t n = len;
        if (data == NULL || len == 0) {
            QM_LOGD(LOG_TAG, "wakeup: (empty)");
            return;
        }
        if (n >= sizeof(text)) {
            n = sizeof(text) - 1;
        }
        memcpy(text, data, n);
        text[n] = '\0';
        QM_LOGD(LOG_TAG, "wakeup: %s", text);
    }
}

static void ver_retry_handle(void *arg)
{
    (void)arg;
    if (g_got_version) {
        return;
    }
    if (g_ver_retry >= 5) {
        QM_LOGD(LOG_TAG, "version query timeout, continue anyway");
        on_chip_ready(NULL);
        return;
    }
    g_ver_retry++;
#if VB701N_APP_DEMO_PLAY
    if (g_ver_retry == 1) {
        qm_audio_capture_policy_t policy = QM_AUDIO_CAPTURE_POLICY_PAUSE;
        qm_audio_control(g_audio, QM_AUDIO_CTRL_SET_CAPTURE_POLICY, &policy);
    }
#endif
    qm_audio_control(g_audio, QM_AUDIO_CTRL_QUERY_VERSION, NULL);
    qm_post_delayed_action(&g_ver_work, ver_retry_handle, NULL, 3000);
}

qm_err_t vb701n_player_start(void)
{
    qm_err_t ret;
    qm_audio_config_t audio_cfg;
    qm_audio_event_cb_t event_callback = on_audio_event;

    QM_LOGD(LOG_TAG, "board uart port %u TX=%u RX=%u baud=%u",
            (unsigned)BOARD_VB701N_UART_PORT, (unsigned)BOARD_VB701N_UART_TX_PIN,
            (unsigned)BOARD_VB701N_UART_RX_PIN, (unsigned)BOARD_VB701N_UART_BAUD);
    ret = qm_work_on_init();
    if (ret != QM_EOK) {
        QM_LOGD(LOG_TAG, "qm_work_on_init fail %d", ret);
        return ret;
    }
    ret = qm_uart_init(&g_board_uart);
    if (ret != QM_EOK) {
        QM_LOGD(LOG_TAG, "qm_uart_init fail %d", ret);
        return ret;
    }
    gpio_set_pull_mode(BOARD_VB701N_UART_RX_PIN, GPIO_PULLUP_ONLY);
    QM_LOGD(LOG_TAG, "rx pin %u level %d",
            (unsigned)BOARD_VB701N_UART_RX_PIN, gpio_get_level(BOARD_VB701N_UART_RX_PIN));
    ret = vb701n_bind(&g_board_uart);
    if (ret != QM_EOK) {
        return ret;
    }
    ret = qm_gpio_init(&g_flow_gpio);
    if (ret != QM_EOK) {
        QM_LOGD(LOG_TAG, "flow gpio init fail %d", ret);
        return ret;
    }
#if VB701N_APP_FLOW_STRESS
    vb701n_set_tx_gate(board_tx_is_busy, 0);
#else
    vb701n_set_tx_gate(board_tx_is_busy, BOARD_VB701N_TX_INTERVAL_MS);
#endif
    QM_LOGD(LOG_TAG, "flow pin %u level %u, tx interval %u ms, stress %d",
            (unsigned)BOARD_VB701N_FLOW_PIN,
            (unsigned)qm_gpio_get_level(&g_flow_gpio),
            (unsigned)(VB701N_APP_FLOW_STRESS ? 0 : BOARD_VB701N_TX_INTERVAL_MS),
            VB701N_APP_FLOW_STRESS);

    memset(&audio_cfg, 0, sizeof(audio_cfg));
#if CONFIG_VOICE_BACKEND_AGORA && VB701N_APP_PCM
    audio_cfg.capture_driver_if = &agora_pcm_bridge_driver;
    audio_cfg.playback_driver_if = &agora_pcm_bridge_driver;
#else
    audio_cfg.capture_driver_if = &vb701n_audio_driver;
    audio_cfg.playback_driver_if = &vb701n_audio_driver;
#endif
    audio_cfg.input_format = &g_in_fmt;
    audio_cfg.output_format = &g_out_fmt;
    g_audio = qm_audio_create(&audio_cfg);
    if (g_audio == NULL) {
        QM_LOGD(LOG_TAG, "qm_audio_create failed");
        return -QM_ENOMEM;
    }
    qm_audio_control(g_audio, QM_AUDIO_CTRL_SET_EVENT_CALLBACK, &event_callback);
    qm_post_delayed_action(&g_ver_work, ver_retry_handle, NULL, 1500);
#if CONFIG_VOICE_BACKEND_AGORA && VB701N_APP_PCM
    QM_LOGI(LOG_TAG, "qm_audio ready, chip pcm, agent opus in %ums out %ums",
            (unsigned)g_in_fmt.frame_ms, (unsigned)g_out_fmt.frame_ms);
#else
    QM_LOGI(LOG_TAG, "qm_audio ready, %s in %ums out %ums",
            VB701N_APP_PCM ? "pcm" : "opus",
            (unsigned)g_in_fmt.frame_ms, (unsigned)g_out_fmt.frame_ms);
#endif
    return QM_EOK;
}

qm_audio_handle_t vb701n_player_get_audio(void)
{
    return g_audio;
}
