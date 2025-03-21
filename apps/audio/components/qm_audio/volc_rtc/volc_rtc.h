/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2025 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "qm.h"

typedef void * volc_rtc_engine_t;

/**
 * @locale zh
 * @type keytype
 * @brief 音频编码类型。
 */
typedef enum {
    /**
     * @locale zh
     * @brief OPUS
     */
    RTC_AUDIO_CODEC_TYPE_OPUS   = 1,
    /**
     * @locale zh
     * @brief G722
     */
    RTC_AUDIO_CODEC_TYPE_G722   = 2,
    /**
    * @locale zh
    * @brief AACLC
    */
    RTC_AUDIO_CODEC_TYPE_AACLC  = 3,

    /**
     * @locale zh
     * @brief G711A
     */
    RTC_AUDIO_CODEC_TYPE_G711A  = 4,
    
} volc_rtc_audio_codec_type_t;

/**
 * @locale zh
 * @type keytype
 * @brief 音频数据类型。
 */
typedef enum {
    /**
     * @locale zh
     * @brief OPUS
     */
    RTC_AUDIO_DATA_TYPE_UNKNOWN = 0,
    /**
     * @locale zh
     * @brief OPUS
     */
    RTC_AUDIO_DATA_TYPE_OPUS    = 1,
    /**
     * @locale zh
     * @brief G722
     */
    RTC_AUDIO_DATA_TYPE_G722    = 2,
    /**
     * @locale zh
     * @brief AACLC
     */
    RTC_AUDIO_DATA_TYPE_AACLC   = 3,
   
    /**
     * @locale zh
     * @brief PCMA
     */
    RTC_AUDIO_DATA_TYPE_PCMA    = 4,
   
} volc_rtc_audio_data_type_t;


/**
 * @locale zh
 * @type keytype
 * @brief 音频帧信息。
 */
typedef struct {
    /**
     * @locale zh
     * @brief 音频数据类型，参看 audio_data_type_e{@link #audio_data_type_e}。
     */
    volc_rtc_audio_data_type_t data_type;

} volc_rtc_audio_frame_info_t;

/**
 * @locale zh
 * @type callback
 * @brief SDK 事件回调结构体
 * @note 回调函数是在 SDK 内部线程同步抛出来的，请不要做耗时操作，否则可能导致 SDK 运行异常。
 */
typedef struct {

    /**
     * @locale zh
     * @type callback
     * @list 回调
     * @order 0
     * @brief SDK 错误信息回调 <br>
     * @param engine 通过byte_rtc_create{@link #byte_rtc_create}创建的引擎实例
     * @param code 错误码，参看 error_code_e{@link #error_code_e}
     * @param msg 错误信息
     */
    void (*on_global_error)(volc_rtc_engine_t engine,int code, const char * msg);
    
    /**
     * @locale zh
     * @type callback
     * @list 回调
     * @order 2
     * @brief 加入房间成功回调。
     * @param engine 通过byte_rtc_create{@link #byte_rtc_create}创建的引擎实例
     * @param room 房间名
     * @param elapsed_ms 从开始加入房间到加入房间成功的耗时，单位：毫秒
     * @param rejoin 当网络断开时，重连后自动触发重新加入房间
     *               - True：重新加入房间
     *               - False：首次加入房间
     */
    void (*on_join_room_success)(volc_rtc_engine_t engine,const char * room, int elapsed_ms, bool rejoin);
    
    
    /**
     * @locale zh
     * @type callback
     * @list 回调
     * @order 1
     * @brief 加入房间失败或异常退出房间回调
     * @param engine 通过byte_rtc_create{@link #byte_rtc_create}创建的引擎实例
     * @param room 房间名
     * @param code 错误码，参看 error_code_e{@link #error_code_e}
     * @param msg 错误信息
     */
    void (*on_room_error)(volc_rtc_engine_t engine,const char * room, int code, const char * msg);
    
    
    /**
     * @locale zh
     * @type callback
     * @list 回调
     * @order 3
     * @brief 远端用户加入房间回调 <br>
              远端用户断网后重新连入房间时，房间内其他用户将收到该事件  <br>
              新加入房间用户也会收到加入房间前已在房间内的用户的入房间回调通知
     * @param engine 通过byte_rtc_create{@link #byte_rtc_create}创建的引擎实例
     * @param room 房间名
     * @param uid 远端用户名
     * @param elapsed_ms 加入房间耗时（保留字段）
     */
    void (*on_user_joined)(volc_rtc_engine_t engine,const char * room, const char * uid,int elapsed_ms);
    
    /**
     * @locale zh
     * @type callback
     * @list 回调
     * @order 4
     * @brief 远端用户离开房间<br>
     *        房间内其他用户会收到此事件
     * @param engine 通过byte_rtc_create{@link #byte_rtc_create}创建的引擎实例
     * @param room 房间名
     * @param uid 远端用户名
     * @param reason 用户离开房间的原因, 参看 user_offline_reason_e{@link #user_offline_reason_e}
     */
    void (*on_user_offline)(volc_rtc_engine_t engine,const char * room, const char * uid , int reason);
    
    /**
     * @locale zh
     * @type callback
     * @list 回调
     * @order 8
     * @brief 房间内用户暂停/恢复发送音频流时，房间内其他用户会收到此回调
     * @param engine 通过byte_rtc_create{@link #byte_rtc_create}创建的引擎实例
     * @param room 房间名
     * @param uid 远端用户名
     * @param muted 发送状态 <br>
    *         - true(1)：不发送 <br>
     *        - false(0)：发送
     */
    void (*on_user_mute_audio)(volc_rtc_engine_t engine,const char * room, const char * uid ,int muted);
    
    /**
     * @locale zh
     * @type callback
     * @list 回调
     * @order 7
     * @brief 房间内用户暂停/恢复发送视频流时，房间内其他用户会收到此回调
     * @param engine 通过byte_rtc_create{@link #byte_rtc_create}创建的引擎实例
     * @param room 房间名
     * @param uid 远端用户名
     * @param muted 发送状态 <br>
    *         - true(1)：不发送 <br>
     *        - false(0)：发送
     */
    void (*on_user_mute_video)(volc_rtc_engine_t engine,const char * room, const char * uid ,int muted);
    
    /**
     * @locale zh
     * @type callback 
     * @list 回调
     * @order 9
     * @brief 提示流发布端需重新生成关键帧的回调
     * @param engine 通过byte_rtc_create{@link #byte_rtc_create}创建的引擎实例
     * @param room 房间名
     * @param uid 远端用户名
     */
    void (*on_key_frame_gen_req)(volc_rtc_engine_t engine,const char * room, const char * uid);
    
    /**
     * @locale zh
     * @type callback
     * @list 回调
     * @order 6
     * @brief 返回远端单个用户的音频数据
     * @param engine 通过byte_rtc_create{@link #byte_rtc_create}创建的引擎实例
     * @param room 房间名
     * @param uid 远端用户名
     * @param sent_ts 发送时间 （暂不支持）
     * @param codec 音频编码类型，参看 audio_codec_type_e{@link #audio_codec_type_e}
     * @param data_ptr 音频数据
     * @param data_len 音频数据长度，单位字节
     */
    void (*on_audio_data)(volc_rtc_engine_t engine,const char * room, const char * uid ,uint16_t sent_ts,
                        volc_rtc_audio_codec_type_t codec, const void * data_ptr, size_t data_len);
#if 0
    /**
     * @locale zh
     * @type callback
     * @list 回调
     * @order 5
     * @brief 返回远端单个用户的视频数据
     * @param engine 通过byte_rtc_create{@link #byte_rtc_create}创建的引擎实例
     * @param room 房间名
     * @param uid  远端用户名
     * @param sent_ts 发送时间（暂不支持）
     * @param codec 视频编码类型，参看 video_data_type_e{@link #video_data_type_e}
     * @param is_key_frame 帧类型
     *                     - 0: 非关键帧
     *                     - !0: 关键帧
     * @param data_ptr 视频数据
     * @param data_len 视频数据长度，单位字节
     */
    void (*on_video_data)(volc_rtc_engine_t engine,const char * room, const char * uid ,uint16_t sent_ts,
                          video_data_type_e codec, int is_key_frame,
                          const void * data_ptr, size_t data_len);
#endif
    
    /**
     * @locale zh
     * @type callback
     * @list 回调
     * @order 10
     * @brief 当带宽估计码率发生变化时，触发该回调。<br>
     *        此时你需要将编码器码率调至目标码率
     * @param engine 通过byte_rtc_create{@link #byte_rtc_create}创建的引擎实例
     * @param room 房间名
     * @param target_bps 目标码率，单位 bps
     */
    void (*on_target_bitrate_changed)(volc_rtc_engine_t engine,const char * room, uint32_t target_bps);
    
    
    /**
     * @locale zh
     * @type callback
     * @list 回调RTC_
     * @order 12
     * @brief 返回远端用户发送的实时信令消息
     * @param engine 通过byte_rtc_create{@link #byte_rtc_create}创建的引擎实例
     * @param room 房间名
     * @param src  远端用户名
     * @param message 实时信令消息
     * @param size 实时信令消息长度
     * @param binary 是否未二进制消息
     */
    
    void (*on_message_received)(volc_rtc_engine_t engine,const char * room,const char * src, const uint8_t * message,int size,bool binary);
    
    
    /**
     * @locale zh
     * @type callback
     * @list 回调
     * @order 11
     * @brief 实时信令消息发送结果通知
     * @param engine 通过byte_rtc_create{@link #byte_rtc_create}创建的引擎实例
     * @param room 房间名
     * @param msgid  发送消息的id，用来和发送的消息匹配
     * @param error 发送消息错误码，0表示发送成功
     * @param extencontent 扩展信息，暂时未使用
     */
    void (*on_message_send_result)(volc_rtc_engine_t engine,const char * room,int64_t msgid, int error,const char * extencontent);
    
    /**
     * @locale zh
     * @type callback
     * @list 回调
     * @order 13
     * @brief Token 加入房间权限过期前 30 秒，触发该回调。<br>
     *        收到该回调后，你需调用 `byte_rtc_renew_token` 更新 Token 加入房间权限
     * @param engine 通过byte_rtc_create{@link #byte_rtc_create}创建的引擎实例
     * @param room 房间名
     */
    void (*on_token_privilege_will_expire)(volc_rtc_engine_t engine,const char * room);
    
    /**
     * @locale zh
     * @type callback
     * @list 回调
     * @order 14
     * @brief license 过期提醒。在剩余天数低于 30 天时，收到此回调
     * @param engine 通过byte_rtc_create{@link #byte_rtc_create}创建的引擎实例
     * @param daysleft license 剩余有效天数
     */
    
    void (*on_license_expire_warning)(volc_rtc_engine_t engine,int daysleft);
    
    /**
     * @locale zh
     * @type callback
     * @list 回调
     * @order 15
     * @brief engine 实例清理(byte_rtc_fini)结束通知，只有收到该通知之后，重新创建实例(byte_rtc_init)才是安全的
     * @param engine 通过byte_rtc_create{@link #byte_rtc_create}创建的引擎实例
     */
    
    void (*on_fini_notify)(volc_rtc_engine_t engine);
    
} volc_rtc_event_handler_t;
    
/**
 * @brief The dispatcher configuration
 */
typedef struct {  
    const char *appid;  
    volc_rtc_event_handler_t event_handler;
} volc_rtc_cfg_t;

/**
 * @brief  Initialize the Volc RTC module.
 *
 * @return
 *     - ESP_OK: Initialization was successful.
 *     - Other: Appropriate erro code indicating the failure reason.
 */
volc_rtc_engine_t volc_rtc_open(volc_rtc_cfg_t *cfg);

/**
 * @brief  Deinitialize the Volc RTC module..
 *
 * @return
 *     - ESP_OK: Deinitialization was successful.
 */
qm_err_t volc_rtc_close(volc_rtc_engine_t engine);

/**
 * @locale zh
 * @type api
 * @list 方法
 * @order 31
 * @brief 更新 Token <br>
 *        收到 on_token_privilege_will_expire{@link #byte_rtc_event_handler_t#on_token_privilege_will_expire} 时，必须重新获取 Token，调用此方法更新 Token，以保证通话的正常进行
 * @param engine 通过byte_rtc_create{@link #byte_rtc_create}创建的引擎实例
 * @param room 房间名
 * @param token 动态密钥。用于对加入房间用户进行鉴权验证  <br>
 * @return 方法调用结果：<br>
 *         - 0：成功 <br>
 *         - -1：引擎实例不存在 <br>
 *         - -2：输入参数为空
 */
qm_err_t volc_rtc_join_room(volc_rtc_engine_t engine,const char * room, const char * uid, const char * token);


/**
 * @locale zh
 * @type api
 * @list 方法
 * @order 21
 * @brief 发送音频帧
 * @param engine 通过byte_rtc_create{@link #byte_rtc_create}创建的引擎实例
 * @param room 房间名
 * @param data_ptr 音频数据
 * @param data_len 数据长度，单位字节
 * @param info_ptr 音频帧信息，参看 audio_frame_info_t{@link #audio_frame_info_t}
 * @return 方法调用结果：<br>
 *         - 0：成功 <br>
 *         - -1：引擎实例不存在 <br>
 *         - -2：输入参数为空
 */
qm_err_t volc_rtc_send_audio_data(volc_rtc_engine_t engine,const char * room,const void * data_ptr, size_t data_len);

#ifdef __cplusplus
}
#endif
