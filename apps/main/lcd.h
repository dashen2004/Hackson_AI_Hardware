#ifndef __LCD_H__
#define __LCD_H__

#include "actions.h"
#include "fonts.h"
#include "screens.h"
#include "styles.h"
#include "ui.h"
#include "vars.h"

#define MUTE_KEY_NAME       "mute"
#define VOLUME_KEY_NAME     "volume"
#define SCR_TMO_KEY_NAME    "scr_tmo"
#define LANG_KEY_NAME       "lang"
#define AWSS_KEY_NAME       "awss"

#define CONFIG_LCD_DISABLE_AUDIO    0
#define CONFIG_LCD_DISABLE_CLOUD    0

void lcd_init(void);

#endif /* __LCD_H__ */
