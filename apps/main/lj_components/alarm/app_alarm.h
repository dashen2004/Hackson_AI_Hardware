#ifndef APP_ALARM_H
#define APP_ALARM_H

#include "qm_errno.h"
#include "qm_types.h"
#include "qm_alarm.h"   /* 平台闹钟组件：本应用层唯一的业务依赖（qm_time/qm_types 由它传递进来） */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 应用层闹钟模块（app_alarm）
 *
 * 只做三件事：
 *   1) 初始化/逆初始化：注册（qm_alarm_create + start）与注销（stop + destroy）平台闹钟组件；
 *   2) 事件搬运：把组件的 4 类通知（响铃/关闭/稍后提醒/内容变化）投递到工作队列，
 *      再在 workqueue 任务上下文里回调模块内置的行为函数（app_alarm_behavior）；
 *   3) 薄转发：把产品层的操作请求转给组件（增删改启停 / 关闭 / 贪睡 / 查询）。
 *
 * 不负责：真实铃声内容、振动、界面 —— 示例用已内嵌的开机音效当提示音，
 * 产品化时替换 app_alarm.c 里的 app_alarm_behavior 即可。
 * 时间（墙钟）：组件内部用 qm_time 自己算下一次触发，应用层不需要引入 timestamp；
 * 只在 NTP/手动对时之后调用 app_alarm_time_changed() 通知组件重新排程。
 */

/*
 * 一次通知的事件快照（值语义：回调返回后仍然有效，可直接保存）。
 * 注意 extensions 不在此转发：它是组件内的借用指针，只在通知回调期间有效；
 * 需要完整条目时，可在行为回调里用 app_alarm_get_list()/get_state() 再查（见下）。
 */
typedef struct {
    qm_alarm_notify_t notify;                   /* QM_ALARM_NOTIFY_RING / DISMISS / SNOOZE / CHANGED */
    uint8_t alarm_id;                           /* 关联闹钟 id，0 表示无 */
    char source[QM_ALARM_SOURCE_MAX_LEN];       /* "device" / "app"，与 alarm_id 组成主键 */
    uint8_t hour;
    uint8_t minute;
    uint8_t repeat_mask;
    qm_bool enabled;
    qm_bool vibrate;
    qm_alarm_tone_t tone;                       /* 铃声选择（LOCAL/ONLINE），播放由产品层实现 */
} app_alarm_event_t;

/*
 * 产品层行为回调。
 * 运行在 workqueue 任务里（不是组件的 owner task），可以安全地调用本模块的接口，
 * 也可以在这里起停止铃声、切页面、再调 app_alarm_close_ringing()/app_alarm_snooze()。
 * 不要在里面长时间阻塞。
 */
typedef void (*app_alarm_behavior_cb_t)(const app_alarm_event_t *evt, void *user);

/*
 * 初始化（= 注册）：创建并启动平台闹钟组件，并在内部登记行为回调
 * （示例实现见 app_alarm.c 的 app_alarm_behavior：响铃 → 抢占音频通路播提示音）。
 * iot_handle 传已有 qm_iot 会话句柄则开启云端同步，传 NULL 只做本地闹钟。
 * 单例：重复调用直接返回 QM_EOK。上层只需调用本函数。
 */
qm_err_t app_alarm_init(void *iot_handle);

/* 逆初始化：停止调度、注销云端方法、销毁组件实例，并清空行为回调。 */
void app_alarm_deinit(void);

/* 是否已初始化（组件实例存在）。 */
qm_bool app_alarm_is_ready(void);

/* ---- 闹钟条目 ---- */

/* 填一份"新增闹钟"可用的默认条目：source=device、id=AUTO(自动分配)、enabled、extensions=NULL。 */
void app_alarm_item_defaults(qm_alarm_item_t *item);

/* 新增（登录）一个本地闹钟；条目里 alarm_id 传 QM_ALARM_LOCAL_ID_AUTO 自动分配。 */
qm_err_t app_alarm_add(const qm_alarm_item_t *item);

/* 修改：item 必须是完整快照，且带有既有的 (source, alarm_id)。 */
qm_err_t app_alarm_modify(const qm_alarm_item_t *item);

/* 删除指定闹钟。 */
qm_err_t app_alarm_delete(const char *source, uint8_t alarm_id);

/* 启用/禁用指定闹钟。 */
qm_err_t app_alarm_enable(const char *source, uint8_t alarm_id, qm_bool enabled);

/* ---- 响铃流程 ---- */

/* 关闭当前正在响铃的闹钟。 */
qm_err_t app_alarm_close_ringing(void);

/* 让当前响铃的闹钟进入稍后提醒（受该闹钟自身 snooze 配置限制）。 */
qm_err_t app_alarm_snooze(void);

/* 取消待响的稍后提醒，回到空闲。 */
qm_err_t app_alarm_cancel_snooze(void);

/* ---- 查询（界面渲染/兜底，不作为触发主路径）---- */

/* 下一个将要触发的闹钟 + 绝对触发时间戳。 */
qm_err_t app_alarm_get_next(qm_alarm_item_t *out, qm_time_t *trigger_time);

/* 整份闹钟列表快照。 */
qm_err_t app_alarm_get_list(qm_alarm_item_t *out, uint16_t capacity, uint16_t *count);

/* 当前运行态（IDLE/RINGING/SNOOZING）与关联的闹钟 id。 */
qm_err_t app_alarm_get_state(qm_alarm_state_t *state, uint8_t *alarm_id);

/* ---- 其它 ---- */

/* NTP/手动对时之后调用：让组件按新时钟重新排程（不调会按旧时钟误触发）。 */
qm_err_t app_alarm_time_changed(void);

/* 主动整表上报云端（一般不需要：本地变更组件会自动上报）。 */
qm_err_t app_alarm_report(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_ALARM_H */
