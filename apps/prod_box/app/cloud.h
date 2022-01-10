#ifndef __CLOUD_H__
#define __CLOUD_H__


#define APP_VERSION  "1.0.2"
#define MCU_VERSION  "1.0.1"

#define USER_PRODUCT_ID   10143
#define USER_PRODUCT_SECRET "52cdc11c600c4936a493a4442746634a"

int is_xjiang_login(void);
int is_netconfig_get(void);
void xjiang_cloud_init(void);

#endif
