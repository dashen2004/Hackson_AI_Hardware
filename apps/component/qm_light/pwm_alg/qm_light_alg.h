#ifndef _QM_LIGHT_ALG_H_
#define _QM_LIGHT_ALG_H_

#include "qm.h"

#ifndef CONFIG_QM_LIGHT_ALG_TBALE_NUM
#define CONFIG_QM_LIGHT_ALG_TBALE_NUM   (101)
#endif

//暂定区间范围应该是10000
#ifndef CONFIG_QM_LIGHT_ALG_FATOR
#define CONFIG_QM_LIGHT_ALG_FATOR   (10000)
#endif

#ifndef CONFIG_QM_LIGHT_GAMMA_FATOR
#define CONFIG_QM_LIGHT_GAMMA_FATOR   (2.13f)
#endif

#ifndef CONFIG_QM_LIGHT_ALG_MAIN_MIN
#define CONFIG_QM_LIGHT_ALG_MAIN_MIN   (100)
#endif

#ifndef CONFIG_QM_LIGHT_ALG_AUX_MIN
#define CONFIG_QM_LIGHT_ALG_AUX_MIN   (100)
#endif

typedef enum 
{
    QM_LIGHT_ALG_ID_MAIN,   //主灯算法
    QM_LIGHT_ALG_ID_AUX,    //辅灯算法
}qm_light_alg_id_t;

void qm_light_alg_table_init(void);

int qm_light_alg_get_exp(qm_light_alg_id_t alg_id, int lightness);

void qm_light_alg_conver_rgb_to_lightness(uint32_t rgb_val, uint16_t *lightness, uint32_t len);

#endif
