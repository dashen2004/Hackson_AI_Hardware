#include "qm.h"
#include "qm_light_alg.h"
#include "qm_light_config.h"
#include "qm_light_driver.h"
// #include "qm_light_storage.h"

typedef struct 
{
    uint16_t main_table;
    uint16_t aux_table;
}qm_light_alg_table_t;

static qm_light_alg_table_t g_light_alg_table[CONFIG_QM_LIGHT_ALG_TBALE_NUM] = {0};

void qm_light_alg_table_init(void)
{
    //生成gamma table
    double temp = 0;
    double temp1 = 0;
    double factor = CONFIG_QM_LIGHT_GAMMA_FATOR;

    memset(&g_light_alg_table, 0, sizeof(qm_light_alg_table_t) * CONFIG_QM_LIGHT_ALG_TBALE_NUM);

    //最低亮度限制
    g_light_alg_table[1].main_table = 1 + CONFIG_QM_LIGHT_ALG_MAIN_MIN;
    g_light_alg_table[1].aux_table = 1 + CONFIG_QM_LIGHT_ALG_AUX_MIN; 

    for (int i = 2; i <= 100; i++)
    {
        temp = 0.5 + 100 * pow((double)(i-1) / 100.0, factor);

        temp1 = (temp + (CONFIG_QM_LIGHT_ALG_MAIN_MIN / 100)) * (CONFIG_QM_LIGHT_ALG_FATOR * 1.0) / 100.0;
        if (temp1 > CONFIG_QM_LIGHT_ALG_FATOR){
            temp1 = CONFIG_QM_LIGHT_ALG_FATOR;
        }
        g_light_alg_table[i].main_table = temp1;

        temp1 = (temp + (CONFIG_QM_LIGHT_ALG_AUX_MIN / 100)) * (CONFIG_QM_LIGHT_ALG_FATOR * 1.0) / 100.0;
        if (temp1 > CONFIG_QM_LIGHT_ALG_FATOR){
            temp1 = CONFIG_QM_LIGHT_ALG_FATOR;
        }
        g_light_alg_table[i].aux_table = temp1;
    }
    //最后一个数据应该是10000
    g_light_alg_table[100].aux_table = CONFIG_QM_LIGHT_ALG_FATOR;
    g_light_alg_table[100].main_table = CONFIG_QM_LIGHT_ALG_FATOR;
}

int qm_light_alg_get_exp(qm_light_alg_id_t alg_id, int lightness)
{
    //查表 g_table, table [0,100]
    //lightness [0,10000]
    //lightness 0~10000, 0->0 10000->10000
    int pwm = 0;
    int index = lightness * 1000 / CONFIG_QM_LIGHT_ALG_FATOR;
    int step = index % 10;

    
    index /= 10;

    if (index >= 100){
        pwm = CONFIG_QM_LIGHT_ALG_FATOR;
    }else{
        if (alg_id == QM_LIGHT_ALG_ID_MAIN){
            pwm = g_light_alg_table[index].main_table + (g_light_alg_table[index + 1].main_table - g_light_alg_table[index].main_table) * step / 10;
        }else if (alg_id == QM_LIGHT_ALG_ID_AUX){
            pwm = g_light_alg_table[index].aux_table + (g_light_alg_table[index + 1].aux_table - g_light_alg_table[index].aux_table) * step / 10;
        }else {
            pwm = lightness;
        }
    }

    return pwm;
}
