/*
进入应用
   ↓
准备平台基础能力
   ↓
运行静态图片姿态识别示例
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
/* #include "bsp_mpu6050.h" */
#include "bsp_servo.h"
#include "bsp_vibrator.h"
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
    example_pose_run();
#endif
    // if (bsp_servo_init() != QM_EOK) {
    //     QM_LOGE("main", "servo init failed"); //初始化舵机
    // }
    // if (bsp_vibrator_init() != QM_EOK) {
    //     QM_LOGE("main", "vibrator init failed"); //初始化马达
    // } else {
    //     bsp_vibrator_pulse(BSP_VIBRATOR_PULSE_MS); //初始化光圈
    // }
    // bsp_ws2812_example();
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
    /* if (bsp_mpu6050_init() != QM_EOK) {
        QM_LOGE("main", "mpu6050 init failed"); //传感器
    } */
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
}
