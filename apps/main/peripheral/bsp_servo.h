#ifndef BSP_SERVO_H
#define BSP_SERVO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "qm.h"

/*
 * 实测脉宽 500us = 0°，2400us = 180°。零位取中点 1450us。
 * 信号 GPIO6，电源按舵机规格供电，舵机地与开发板共地。
 */
#define BSP_SERVO_PWM_PIN           6
#define BSP_SERVO_PWM_HZ            50
#define BSP_SERVO_PWM_RES_BIT       14

#define BSP_SERVO_PULSE_MIN_US      500
#define BSP_SERVO_PULSE_MAX_US      2400

#define BSP_SERVO_PULSE_ANGLE_MIN   0
#define BSP_SERVO_PULSE_ANGLE_MAX   180

/* 500 和 2400 的中点。改这个值就是改上电停的位置。 */
#define BSP_SERVO_ZERO_PULSE_US     1450

#define BSP_SERVO_MOVE_STEP_DEG         20
#define BSP_SERVO_MOVE_STEP_MS          20
#define BSP_SERVO_WIGGLE_STEP_MS        60

qm_err_t bsp_servo_init(void);
void bsp_servo_deinit(void);

qm_err_t bsp_servo_set_pulse_us(uint16_t pulse_us);
uint16_t bsp_servo_get_pulse_us(void);

qm_err_t bsp_servo_set_angle(int16_t angle);
int16_t bsp_servo_get_angle(void);
qm_err_t bsp_servo_wiggle(int16_t delta_deg, uint32_t cycles);

#ifdef __cplusplus
}
#endif

#endif
