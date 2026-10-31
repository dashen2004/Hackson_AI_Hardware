/*
进入应用
   ↓
准备平台基础能力
   ↓
准备摄像头姿态识别（关闭摄像头时才运行静态图片示例）
   ↓
初始化工作队列
   ↓
启动板级 IO 监测
   ↓
准备语音、唤醒及开机音
   ↓
准备资源播放
   ↓
准备云接入
   ↓
接入物模型控制和课程表
   ↓
启动配网相关流程
   ↓
接入智能体应用
   ↓
准备闹钟
   ↓
启动函数结束，后台任务和事件处理继续运行
*/


#include "qm.h"
#include "qm_work.h"
#include "bsp_io.h"
#include "bsp_servo.h"
#include "bsp_touch.h"
#include "bsp_ws2812.h"
#include "voice.h"
#include "agent_app.h"
#include "ljyd_cloud.h"
#include "ljyd_provision.h"
#include "iot_control.h"
#include "pomodoro.h"
#include "ljyd_course.h"
#if defined(CONFIG_QM_POSE_SUPPORT) && CONFIG_QM_POSE_SUPPORT
#include "example_pose.h"
#if defined(CONFIG_APP_CAMERA_TEST) && CONFIG_APP_CAMERA_TEST
#include "camera_pose.h"
#endif
#endif

#if defined(CONFIG_APP_CAMERA_TEST) && CONFIG_APP_CAMERA_TEST
#include "bsp_camera.h"
#endif

#if defined(CONFIG_QM_ALARM_SUPPORT) && CONFIG_QM_ALARM_SUPPORT
#include "app_alarm.h"
#endif

#if defined(CONFIG_LJYD_PLAYBACK_SUPPORT) && CONFIG_LJYD_PLAYBACK_SUPPORT
#include "playback.h"
#endif

#include <stdio.h>
#include <stdarg.h>

static void touch_press_handler(void *arg)
{
    (void)arg;
    (void)bsp_servo_wiggle(40, 8);
}

int qm_vsnprintf(char *str, size_t count, const char *fmt, va_list ap)
{
    return vsnprintf(str, count, fmt, ap);
}

void app_main(void)
{
    qm_main();
}

void qm_application_start(void)
{
#if defined(CONFIG_QM_POSE_SUPPORT) && CONFIG_QM_POSE_SUPPORT
#if defined(CONFIG_APP_CAMERA_TEST) && CONFIG_APP_CAMERA_TEST
    /* 模型与工作缓冲先准备好，摄像头取流后不再加载 Flash 模型。 */
    if (camera_pose_init() != ESP_OK) {
        QM_LOGE("main", "camera pose init failed; capture diagnostics remain available");
    }
#else
    example_pose_run();
#endif
#endif
    if (bsp_servo_init() != QM_EOK) {
        QM_LOGE("main", "servo init failed");
    }
    bsp_touch_set_press_callback(touch_press_handler, NULL);
    if (bsp_touch_init() != QM_EOK) {
        QM_LOGE("main", "touch input init failed");
    }
    // 上电联调：8 颗灯低亮度绿灯常亮，不依赖联网。
    if (bsp_ws2812_init() != QM_EOK) {
        QM_LOGE("main", "ws2812 init failed");
    } else if (bsp_ws2812_fill(0, 16, 0) != QM_EOK) {
        QM_LOGE("main", "ws2812 startup fill failed");
    } else {
        QM_LOGI("main", "ws2812 startup green ok gpio%d leds %d level 16",
                BSP_WS2812_GPIO, BSP_WS2812_LED_NUM);
    }
    if (qm_work_on_init() != QM_EOK) {
        QM_LOGE("main", "qm_work_on_init failed"); //初始化工作队列
        return;
    }
    if (bsp_io_init() != QM_EOK) {
        QM_LOGE("main", "io init failed"); //初始化板级IO（工作队列可以先理解为一个“待办事项处理机制”）
        return;
    }
#if defined(CONFIG_APP_CAMERA_TEST) && CONFIG_APP_CAMERA_TEST
    if (bsp_camera_early_init() != ESP_OK) {
        QM_LOGE("main", "camera early init failed"); //摄像头
    }
#endif
    if (voice_start() != QM_EOK) {
        QM_LOGE("main", "audio init failed"); //voice_start() 会准备音频模块、订阅唤醒事件、创建音频控制对象、恢复音量，并启动开机音播放。
        return;
    }
#if defined(CONFIG_LJYD_PLAYBACK_SUPPORT) && CONFIG_LJYD_PLAYBACK_SUPPORT
    if (playback_init() != QM_EOK) {
        QM_LOGE("main", "playback init failed"); //初始化资源播放：资源播放可以先理解成：播放云端下发的音频资源等内容
    }
#endif

    if (ljyd_cloud_init() != QM_EOK) {
        QM_LOGE("main", "ljyd_cloud_init failed"); //接着初始化云接入：它准备设备注册、身份信息、云端通信和事件处理等能力。
        return;
    }
    if (iot_control_init(ljyd_cloud_get_handle()) != QM_EOK) {
        QM_LOGE("main", "iot_control_init failed"); //准备接收云端属性控制，例如设置音量。
    }
    if (ljyd_course_init(ljyd_cloud_get_handle()) != QM_EOK) {
        QM_LOGE("main", "ljyd_course_init failed"); //初始化课程表模块，让它能够使用云接入对象同步、处理课程表相关数据。
    }
    if (ljyd_provision_init() != QM_EOK) {
        QM_LOGE("main", "ljyd_provision_init failed"); //初始化配网流程，处理已有 Wi-Fi 配置、需要配网等情况。
    }
    if (agent_app_init(ljyd_cloud_get_handle()) != QM_EOK) {
        QM_LOGE("main", "agent_app_init failed"); //初始化智能体应用，把云连接事件、语音唤醒事件和智能体会话连接起来。
    }
#if defined(CONFIG_QM_ALARM_SUPPORT) && CONFIG_QM_ALARM_SUPPORT
    /* 闹钟：开机即初始化（不要求已联网）。云端同步与响铃行为注册都在模块内部完成。 */
    if (app_alarm_init(ljyd_cloud_get_handle()) != QM_EOK) {
        QM_LOGE("main", "app_alarm_init failed");
    }
#endif

#if defined(CONFIG_QM_POMODORO_SUPPORT) && CONFIG_QM_POMODORO_SUPPORT
    pomodoro_init(); //番茄钟
#endif

#if defined(CONFIG_APP_CAMERA_TEST) && CONFIG_APP_CAMERA_TEST
    /* 本地坐姿检测不依赖 Wi-Fi/云端；所有启动初始化完成后再采集。 */
    if (bsp_camera_stream_start() != ESP_OK) {
        QM_LOGE("main", "camera stream start failed");
    }
#endif
}
