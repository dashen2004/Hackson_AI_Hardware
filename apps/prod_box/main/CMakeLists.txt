idf_component_register(SRCS "network.c" "sys_notify.c" "main.c"
                    INCLUDE_DIRS "." REQUIRES esp32x nvs_flash app)
