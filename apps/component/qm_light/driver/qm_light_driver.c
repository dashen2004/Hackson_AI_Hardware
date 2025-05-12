#include "qm.h"
#include "qm_pwm.h"
#include "qm_light_config.h"
#include "qm_light_driver.h"

#define LOG_TAG "light_driver"

#if 0
static ectimer_t light_blink_timer;
#endif

typedef struct
{
    int32_t current_lightness; 
    int32_t target_lightness;
    uint16_t steps;
    int32_t step_delta;
} light_lightness_action_t;

typedef struct
{
    uint16_t lightness_begin;
    uint16_t lightness_end;
    bool in_begin_phase;
    uint32_t begin_count;
    uint32_t begin_cur_count;
    uint32_t end_count;
    uint32_t end_cur_count;
    uint32_t times;
} light_blink_action_t;

typedef struct
{
    uint16_t lightness_begin;
    uint16_t lightness_end;
    uint16_t current_lightness;
    bool_t in_forward_phase;
    uint8_t current_forward_steps;
    uint8_t forward_steps;
    int32_t forward_step_delta;
    uint8_t current_reverse_steps;
    uint8_t reverse_steps;
    int32_t reverse_step_delta;
    uint32_t times;
    bool_t half_breath_end;
} light_breath_action_t;

typedef enum
{
    LIGHT_ACTION_LIGHTNESS_LINEAR,
    LIGHT_ACTION_BLINK,
    LIGHT_ACTION_BREATH,
    LIGHT_ACTION_UNKNOWN,
} light_action_type_t;

typedef union
{
    light_lightness_action_t lightness;
    light_blink_action_t blink;
    light_breath_action_t breath;
} light_action_value_t;

typedef struct
{
    bool_t busy;
    bool_t need_change;
    qm_light_driver_handle_t light;
    light_action_type_t type;
    light_action_value_t value;
} light_action_t;

typedef struct
{
    uint32_t freq;
    qm_pwm_dev_t dev;
    uint16_t lightness;         //默认亮度值，有效范围[0,65535]
    qm_light_driver_level_t level;     //行为电平
    qm_light_driver_event_cb cb;
    void *arg;
} light_handle_t;

static qm_timer_t  light_ctl_timer = {0};
static light_action_t light_actions[CONFIG_QM_LIGHT_MAX_ACTION_NUM] = {0};

static light_action_t *request_action(qm_light_driver_handle_t light)
{
    /* find same first */
    for (uint8_t i = 0; i < CONFIG_QM_LIGHT_MAX_ACTION_NUM; ++i)
    {
        if ((light == light_actions[i].light) && (light_actions[i].busy))
        {
            return &light_actions[i];
        }
    }

    /* no same, find empty */
    for (uint8_t i = 0; i < CONFIG_QM_LIGHT_MAX_ACTION_NUM; ++i)
    {
        if (!light_actions[i].busy){
            light_actions[i].busy = QM_TRUE;
            return &light_actions[i];
        }
    }

    return NULL;
}

static void release_action(light_action_t *action, qm_light_driver_event_type_t type)
{
    action->busy = QM_FALSE;
    light_handle_t *handle = (light_handle_t *)action->light;

    if (NULL != handle->cb) {
        handle->cb(handle, type, handle->arg);
    }
}

static bool_t is_all_light_idle(void)
{
    for (uint8_t channel = 0; channel < CONFIG_QM_LIGHT_MAX_ACTION_NUM; ++channel)
    {
        if (light_actions[channel].busy && light_actions[channel].need_change)
        {
            return QM_FALSE;
        }
    }
    return QM_TRUE;
}

static void light_lighten(qm_light_driver_handle_t handle, uint16_t lightness)
{
    double pulse = 0;
    qm_pwm_config_t cfg = {0};
    light_handle_t *light_handle = (light_handle_t *)handle;
    //将lightness [0,65535] 转换为pulse [0, period]

    light_handle->lightness = lightness;

    cfg.freq = light_handle->freq;
    pulse = (double)lightness * (double)(CONFIG_QM_LIGHT_PULSE_PERIOD * 1.0) / (double)(QM_LIGHT_LIGHTNESS_MAX_VALUE * 1.0);
    if (light_handle->level){
        cfg.pulse = (uint32_t)pulse;
    }else{
        cfg.pulse = QM_LIGHT_LIGHTNESS_MAX_VALUE - pulse;
    }
    // QM_LOGD(LOG_TAG,"[en light]:[%u][%u]\n", lightness, cfg.pulse);
    
    qm_pwm_para_change(&light_handle->dev, cfg);
}

static void light_controller_timer_start(void)
{
    if (qm_timer_is_valid(&light_ctl_timer)){
        qm_timer_start(&light_ctl_timer);
    }
}

static void light_ctl_timeout_handle(qm_timer_t *timer,  void *arg)
{
    // UNUSED(pargs);
    //底层调度可能时间不准，每次需要检测调度间隔，根据间隔来判断需要执行多少step
    int adj_step = 1;
#if CONFIG_QM_LIGHT_CHECK_GRADIENT_INTERVAL
    static uint32_t  _last_time = 0;
    uint32_t time_ms = qm_now_ms();
    if (0 == _last_time){
        _last_time = time_ms;
    }else{
        adj_step = ((time_ms - _last_time) / CONFIG_QM_LIGHT_MONITOR_INTERVAL);
        if (adj_step < 1){
            adj_step = 1;
        }
        _last_time = time_ms;
    }
#endif
    for (uint8_t channel = 0; channel < CONFIG_QM_LIGHT_MAX_ACTION_NUM; ++channel)
    {
        if (light_actions[channel].busy && light_actions[channel].need_change)
        {
            switch (light_actions[channel].type)
            {
            case LIGHT_ACTION_LIGHTNESS_LINEAR:
                if (light_actions[channel].value.lightness.steps > 1){

                    light_lighten(light_actions[channel].light,
                    light_actions[channel].value.lightness.current_lightness);

                    light_actions[channel].value.lightness.current_lightness += 
                        (light_actions[channel].value.lightness.step_delta * adj_step);

                    if(light_actions[channel].value.lightness.current_lightness <= 0){
                        light_actions[channel].value.lightness.current_lightness = 0;
                        light_actions[channel].value.lightness.steps = 1;
                    }else{
                        light_actions[channel].value.lightness.steps -= adj_step;
                    }

                }else{

                    light_lighten(light_actions[channel].light,
                                  light_actions[channel].value.lightness.target_lightness);
                    #if 0
                    if (light_actions[channel].value.lightness.target_lightness > 0)
                    {
//                        DBG_DIRECT("[Light] channel: %d  last: %d  target %d\r\n", channel, 
//                            light_actions[channel].light->lightness_last, light_actions[channel].value.lightness.target_lightness);
                        light_actions[channel].light->lightness_last =
                            light_actions[channel].value.lightness.target_lightness;
                    }
                    #endif

                    light_actions[channel].need_change = QM_FALSE;
                    release_action(&light_actions[channel], QM_LIGHT_DRIVER_EVNET_COMPLETE_LIGHTNESS_LINEAR);

                }
                break;
            case LIGHT_ACTION_BLINK:
                if (light_actions[channel].value.blink.times > 0)
                {
                    if (light_actions[channel].value.blink.in_begin_phase)
                    {
                        if (light_actions[channel].value.blink.begin_cur_count >=
                            light_actions[channel].value.blink.begin_count)
                        {
                            light_actions[channel].value.blink.in_begin_phase = QM_FALSE;
                            light_actions[channel].value.blink.begin_cur_count = 0;
                        }
                        else
                        {
                            if (0 == light_actions[channel].value.blink.begin_cur_count)
                            {
                                light_lighten(light_actions[channel].light,
                                              light_actions[channel].value.blink.lightness_begin);
                            }
                            light_actions[channel].value.blink.begin_cur_count += adj_step;
                        }
                    }
                    else
                    {
                        if (light_actions[channel].value.blink.end_cur_count >=
                            light_actions[channel].value.blink.end_count)
                        {
                            light_actions[channel].value.blink.in_begin_phase = QM_TRUE;
                            light_actions[channel].value.blink.end_cur_count = 0;

                            if (QM_LIGHT_TIME_INFINITE != light_actions[channel].value.blink.times)
                            {
                                light_actions[channel].value.blink.times--;
                            }
                        }
                        else
                        {
                            if (0 == light_actions[channel].value.blink.end_cur_count)
                            {
                                light_lighten(light_actions[channel].light, light_actions[channel].value.blink.lightness_end);
                            }
                            light_actions[channel].value.blink.end_cur_count += adj_step;
                        }
                    }
                }
                else
                {
                    light_actions[channel].need_change = QM_FALSE;
                    release_action(&light_actions[channel], QM_LIGHT_DRIVER_EVNET_COMPLETE_BLINK);
                }
                break;
            case LIGHT_ACTION_BREATH:

                if (light_actions[channel].value.breath.times > 0)
                {
                    if (light_actions[channel].value.breath.in_forward_phase)
                    {
                        if (light_actions[channel].value.breath.current_forward_steps >=
                            light_actions[channel].value.breath.forward_steps)
                        {
                            light_lighten(light_actions[channel].light,
                                          light_actions[channel].value.breath.lightness_end);
                            light_actions[channel].value.breath.current_lightness =
                                light_actions[channel].value.breath.lightness_end;
                            light_actions[channel].value.breath.current_reverse_steps = 0;
                            light_actions[channel].value.breath.in_forward_phase = QM_FALSE;
                            if (1 == light_actions[channel].value.breath.times)
                            {
                                if (light_actions[channel].value.breath.half_breath_end)
                                {
                                    light_actions[channel].value.breath.times = 0;
                                    light_actions[channel].need_change = QM_FALSE;
                                    release_action(&light_actions[channel], QM_LIGHT_DRIVER_EVNET_COMPLETE_BREATH);
                                }
                            }
                        }
                        else
                        {
                            light_lighten(light_actions[channel].light,
                                          light_actions[channel].value.breath.current_lightness);
                            light_actions[channel].value.breath.current_lightness +=
                                light_actions[channel].value.breath.forward_step_delta * adj_step;
                            light_actions[channel].value.breath.current_forward_steps += adj_step;
                        }
                    }
                    else
                    {
                        if (light_actions[channel].value.breath.current_reverse_steps >=
                            light_actions[channel].value.breath.reverse_steps)
                        {
                            light_lighten(light_actions[channel].light,
                                          light_actions[channel].value.breath.lightness_begin);
                            light_actions[channel].value.breath.current_lightness =
                                light_actions[channel].value.breath.lightness_begin;
                            light_actions[channel].value.breath.current_forward_steps = 0;
                            light_actions[channel].value.breath.in_forward_phase = QM_TRUE;

                            if (QM_LIGHT_TIME_INFINITE != light_actions[channel].value.breath.times)
                            {
                                light_actions[channel].value.breath.times --;
                            }
                        }
                        else
                        {
                            light_lighten(light_actions[channel].light,
                                          light_actions[channel].value.breath.current_lightness);
                            light_actions[channel].value.breath.current_lightness +=
                                light_actions[channel].value.breath.reverse_step_delta * adj_step;
                            light_actions[channel].value.breath.current_reverse_steps += adj_step;
                        }
                    }
                }
                else
                {
                    light_actions[channel].need_change = QM_FALSE;
                    release_action(&light_actions[channel], QM_LIGHT_DRIVER_EVNET_COMPLETE_BREATH);
                }
                
                break;
            default:
                break;
            }
        }
    }

    /* Check light controler status */
    if (!is_all_light_idle())
    {
        light_controller_timer_start();
    }
}


qm_light_driver_handle_t qm_light_driver_init(qm_light_driver_param_t *param)
{
    light_handle_t *light_handle = (light_handle_t *)qm_malloc(sizeof(light_handle_t));
    if(light_handle == NULL){
        return NULL;
    }
    memset(light_handle, 0, sizeof(light_handle_t));

    light_handle->cb = param->cb;
    light_handle->arg = param->arg;
    light_handle->lightness = param->lightness;
    light_handle->level = param->level;

    /**PWM init**/
    light_handle->dev.port = param->port;
    light_handle->dev.pin.pwm_pin = param->pin;
    light_handle->dev.config.freq = param->freq;
    light_handle->dev.config.pulse = param->lightness;
    qm_pwm_init(&light_handle->dev);

    if(!qm_timer_is_valid(&light_ctl_timer)){
        qm_timer_new(&light_ctl_timer, light_ctl_timeout_handle, NULL, CONFIG_QM_LIGHT_MONITOR_INTERVAL, 0);
    }

    qm_pwm_start(&light_handle->dev);

    return (qm_light_driver_handle_t)light_handle;
}

qm_err_t qm_light_driver_stop(qm_light_driver_handle_t handle)
{
    light_handle_t *light_handle = NULL;
    for (uint8_t i = 0; i < CONFIG_QM_LIGHT_MAX_ACTION_NUM; ++i)
    {
        if (light_actions[i].light == handle)
        {
            light_actions[i].type = LIGHT_ACTION_UNKNOWN;
            light_actions[i].need_change = QM_FALSE;
            light_actions[i].busy = QM_FALSE;
            release_action(&light_actions[i], QM_LIGHT_DRIVER_EVNET_COMPLETE_STOP);
            break;
        }
    }
    return QM_EOK;
}

qm_err_t qm_light_driver_set_lightness_linear(qm_light_driver_handle_t handle, uint16_t lightness, uint32_t time)
{
#if 0
#if defined(FLEX_SWITCH_DIRECT_OFF) && FLEX_SWITCH_DIRECT_OFF == 1
    if (0 == time)
    {
        //zero time, so do immediately
        light_stop(light);
        light_lighten(light, lightness);
        if (pcb)
        {
            pcb(light);
        }
        return;
    }
#endif
#if 0
    //如果当前lightness和目标lightness一致，则不需要再次设置
    if (light->lightness == lightness)
    {
        return;
    }
#endif
#endif
    light_action_t *paction = request_action(handle);
    light_handle_t *light_handle = (light_handle_t *)handle;

    if (NULL == paction){
        return -QM_EINVAL;
    }

    paction->light = handle;
    paction->type = LIGHT_ACTION_LIGHTNESS_LINEAR;
    paction->value.lightness.current_lightness = light_handle->lightness;
    paction->value.lightness.target_lightness = lightness;
    paction->value.lightness.steps = time / CONFIG_QM_LIGHT_MONITOR_INTERVAL;
    if (0 == paction->value.lightness.steps)
    {
        paction->value.lightness.steps = 1;
    }
    paction->value.lightness.step_delta = (int32_t)(paction->value.lightness.target_lightness - paction->value.lightness.current_lightness) / paction->value.lightness.steps;
    
    paction->need_change = QM_TRUE;

    // QM_LOGD(LOG_TAG,"[en step_delta]:%d\n", paction->value.lightness.step_delta);

    light_controller_timer_start();

    return QM_EOK;
}

qm_err_t qm_light_driver_blink(qm_light_driver_handle_t handle, uint16_t lightness_begin, uint16_t lightness_end,
                 uint32_t interval, uint8_t begin_duty, uint32_t times)
{
    light_action_t *paction = request_action(handle);
    light_handle_t *light_handle = (light_handle_t *)handle;
    uint32_t total_count = interval / CONFIG_QM_LIGHT_MONITOR_INTERVAL + 1;

    if (begin_duty > 100){
        begin_duty = 100;
    }

    if (NULL == paction){
        return -QM_EINVAL;
    }

    paction->light = handle;
    paction->type = LIGHT_ACTION_BLINK;
    paction->value.blink.lightness_begin = lightness_begin;
    paction->value.blink.lightness_end = lightness_end;
    paction->value.blink.in_begin_phase = QM_TRUE;
    paction->value.blink.begin_count = total_count * begin_duty / 100;
    paction->value.blink.begin_cur_count = 0;
    paction->value.blink.end_count = total_count - paction->value.blink.begin_count;
    paction->value.blink.end_cur_count = 0;
    paction->value.blink.times = times;
    paction->need_change = QM_TRUE;

    light_controller_timer_start();

    return QM_EOK;
}

qm_err_t qm_light_driver_breath(qm_light_driver_handle_t handle, uint16_t lightness_begin, uint16_t lightness_end,
                    uint32_t interval, uint8_t forward_duty, uint32_t times, bool_t half_breath_end)
{
    light_action_t *paction = request_action(handle);
    light_handle_t *light_handle = (light_handle_t *)handle;
    uint32_t total_steps = interval / CONFIG_QM_LIGHT_MONITOR_INTERVAL;

    if (NULL == paction){
        return -QM_EINVAL;
    }

    if (0 == total_steps){
        total_steps = 1;
    }

    if (forward_duty > 100) {
        forward_duty = 100;
    }

    paction->light = handle;
    paction->type = LIGHT_ACTION_BREATH;
    paction->value.breath.lightness_begin = lightness_begin;
    paction->value.breath.lightness_end = lightness_end;
    paction->value.breath.current_lightness = lightness_begin;
    paction->value.breath.in_forward_phase = QM_TRUE;
    paction->value.breath.current_forward_steps = 0;
    paction->value.breath.forward_steps = total_steps * forward_duty / 100;
    paction->value.breath.forward_step_delta = (int32_t)(lightness_end - lightness_begin) /
                                                paction->value.breath.forward_steps;
    paction->value.breath.current_reverse_steps = 0;
    paction->value.breath.reverse_steps = total_steps -
                                            paction->value.breath.forward_steps;
    paction->value.breath.reverse_step_delta = (int32_t)(lightness_begin - lightness_end) /
                                                paction->value.breath.reverse_steps;
    paction->value.breath.times = times;
    paction->value.breath.half_breath_end = half_breath_end;
    paction->need_change = QM_TRUE;

    light_controller_timer_start();

    return QM_EOK;
}

bool_t qm_light_driver_is_busy(const qm_light_driver_handle_t light)
{
    for (uint8_t i = 0; i < CONFIG_QM_LIGHT_MAX_ACTION_NUM; ++i)
    {
        if (light == light_actions[i].light)
        {
            return light_actions[i].busy;
        }
    }

    return QM_FALSE;
}
