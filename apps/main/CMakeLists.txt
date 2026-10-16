if(DEFINED ENV{ADF_PATH} AND EXISTS "$ENV{ADF_PATH}/qumios-dev")
    set(QDEV "$ENV{ADF_PATH}/qumios-dev")
else()
    get_filename_component(QDEV "${CMAKE_CURRENT_LIST_DIR}/../../qumios-dev" ABSOLUTE)
endif()
file(TO_CMAKE_PATH "${QDEV}" QDEV)
if(NOT IS_DIRECTORY "${QDEV}")
    message(FATAL_ERROR "qumios-dev not found at ${QDEV}. Set ADF_PATH to the repo root (esp32_combo).")
endif()
set(AGORA_SDK ${CMAKE_CURRENT_LIST_DIR}/../../components/agora_iot_sdk)

file(READ "${CMAKE_CURRENT_LIST_DIR}/qm_config/qm_config.h" QM_CONFIG_H)
if(QM_CONFIG_H MATCHES "#define[ \t]+CONFIG_VOICE_BACKEND_AGORA[ \t]+1")
    set(VOICE_BACKEND_AGORA 1)
else()
    set(VOICE_BACKEND_AGORA 0)
endif()
message(STATUS "voice backend agora=${VOICE_BACKEND_AGORA}")

set(COMPONENT_SRCS
    "./main.c"
    "./agent/agent_app.c"
    "./cloud/ljyd_cloud.c"
    "./cloud/ljyd_method.c"
    "./cloud/ljyd_provision.c"
    "./cloud/ljyd_iot_prop.c"
    "./cloud/ljyd_iot_volume.c"
    "./peripheral/bsp_uart.c"
    "./peripheral/bsp_io.c"
    "./peripheral/bsp_mpu6050.c"
    "./peripheral/bsp_servo.c"
    "./peripheral/bsp_vibrator.c"
    "./peripheral/bsp_ws2812.c"
    "./voice/voice.c"

    "${QDEV}/components/qm_agent/src/core/qm_agent_core.c"
    "${QDEV}/components/qm_agent/src/core/qm_agent_event.c"
    "${QDEV}/components/qm_agent/src/core/qm_agent_module.c"
    "${QDEV}/components/qm_agent/src/audio/qm_agent_audio_engine.c"
    "${QDEV}/components/qm_agent/src/auth/qm_agent_auth.c"
    "${QDEV}/components/qm_agent/src/transport/qm_agent_transport.c"
    "${QDEV}/components/qm_websocket_client/src/qm_websocket_client.c"
    "${QDEV}/components/qm_websocket_client/src/qm_websocket_transport.c"
    "${QDEV}/components/qm_time/src/qm_time.c"
    "${QDEV}/platform/mcu/esp32s3/hal/soc/qm_rtc.c"
    "${QDEV}/utility/uuid/qm_utils_uuid.c"
    "${QDEV}/utility/sprintf/qm_sprintf.c"

    "${QDEV}/drivers/VB701N/src/vb701n.c"
    "${QDEV}/drivers/VB701N/proto/vb701n_proto.c"
    "${QDEV}/components/qm_audio/src/qm_audio.c"
    "${QDEV}/components/qm_audio/src/qm_audio_capture.c"
    "${QDEV}/components/qm_audio/src/qm_audio_playback.c"
    "${QDEV}/components/qm_event/src/qm_event.c"
    "${QDEV}/components/qm_event/src/qm_work.c"
    "${QDEV}/components/qm_provision/src/qm_provision.c"
    "${QDEV}/components/qm_cloud/api/qm_iot_api.c"
    "${QDEV}/components/qm_cloud/config/qm_iot_config.c"
    "${QDEV}/components/qm_cloud/dev/qm_iot_dev.c"
    "${QDEV}/components/qm_cloud/dynreg/qm_iot_dynreg.c"
    "${QDEV}/components/qm_cloud/mqtt/qm_iot_mqtt.c"
    "${QDEV}/components/qm_cloud/pub/qm_iot_pub.c"
    "${QDEV}/components/qm_cloud/shadow/qm_iot_shadow.c"
    "${QDEV}/components/qm_spec/src/qm_spec_api.c"
    "${QDEV}/components/qm_spec/src/qm_spec_core.c"
    "${QDEV}/components/qm_ble/api/qm_ble_async.c"
    "${QDEV}/components/qm_ble/api/qm_ble_log.c"
    "${QDEV}/components/qm_ble/api/qm_ble_export.c"
    "${QDEV}/components/qm_ble/core/qm_ble_auth.c"
    "${QDEV}/components/qm_ble/core/qm_ble_core.c"
    "${QDEV}/components/qm_ble/core/qm_ble_service.c"
    "${QDEV}/components/qm_ble/core/qm_ble_transport.c"
    "${QDEV}/components/qm_ble/core/qm_ble_utils.c"
    "${QDEV}/components/qm_db/src/qm_kv.c"
    "${QDEV}/components/qm_db/src/qm_db_log.c"
    "${QDEV}/components/wifi_manager/src/qm_wifi_mgnt.c"
    "${QDEV}/components/wifi_manager/src/qm_wifi_dev.c"
    "${QDEV}/components/qm_http_client/src/qm_http_client.c"
    "${QDEV}/components/qm_http_client/src/qm_http_client_transport.c"
    "${QDEV}/components/http/util_httpc.c"
    "${QDEV}/components/network/util_net.c"
    "${QDEV}/components/mqtt/iot_mqtt_client.c"
    "${QDEV}/components/mqtt/MQTTPacket/MQTTConnectClient.c"
    "${QDEV}/components/mqtt/MQTTPacket/MQTTDeserializePublish.c"
    "${QDEV}/components/mqtt/MQTTPacket/MQTTPacket.c"
    "${QDEV}/components/mqtt/MQTTPacket/MQTTSerializePublish.c"
    "${QDEV}/components/mqtt/MQTTPacket/MQTTSubscribeClient.c"
    "${QDEV}/components/mqtt/MQTTPacket/MQTTUnsubscribeClient.c"
    "${QDEV}/components/backoff/src/backoff_algorithm.c"
    "${QDEV}/components/cjson/cJSON.c"
    "${QDEV}/utility/log/qm_log.c"
    "${QDEV}/utility/string/qm_string.c"
    "${QDEV}/utility/string/qm_utils_string.c"
    "${QDEV}/utility/list/qm_utils_list.c"
    "${QDEV}/utility/timer/qm_utils_timer.c"
    "${QDEV}/utility/json_parser/json_parser.c"
    "${QDEV}/utility/hmac/qm_utils_hmac.c"
    "${QDEV}/utility/base64/qm_utils_base64.c"
    "${QDEV}/utility/md5/qm_utils_md5.c"
    "${QDEV}/utility/sha256/qm_utils_sha256.c"
    "${QDEV}/utility/sha1/qm_utils_sha1.c"
    "${QDEV}/utility/aes/qm_utils_aes.c"
    "${QDEV}/components/qm_time/src/qm_time.c"
    "${QDEV}/platform/mcu/esp32s3/qmos/qmos_freertos.c"
    "${QDEV}/platform/mcu/esp32s3/qmos/qm_main.c"
    "${QDEV}/kernel/init/qm_init.c"
    "${QDEV}/platform/mcu/esp32s3/hal/soc/qm_uart.c"
    "${QDEV}/platform/mcu/esp32s3/hal/soc/qm_gpio.c"
    "${QDEV}/platform/mcu/esp32s3/hal/soc/qm_pwm.c"
    "${QDEV}/platform/mcu/esp32s3/hal/wifi/qm_wifi.c"
    "${QDEV}/platform/mcu/esp32s3/hal/net/qm_tcp.c"
    "${QDEV}/platform/mcu/esp32s3/hal/net/qm_tls.c"
    "${QDEV}/platform/mcu/esp32s3/hal/ble/qm_ble_gap.c"
    "${QDEV}/platform/mcu/esp32s3/qm_ble/qm_ble_hal_ble.c"
    "${QDEV}/platform/mcu/esp32s3/qm_ble/qm_ble_hal_os.c"
    "${QDEV}/platform/mcu/esp32s3/qm_ble/qm_ble_hal_ota.c"
    "${QDEV}/platform/mcu/esp32s3/qm_ble/qm_ble_hal_sec.c"
    "${QDEV}/platform/mcu/esp32s3/qm_db/qm_hal_kv.c"
)

list(APPEND COMPONENT_SRCS
    "./agent/private/opus.c"
    "./agent/private/pcm_drv.c"
    "${QDEV}/components/qm_agent/src/transport/qm_agent_transport_agora.c"
    "${QDEV}/components/qm_agent/src/transport/qm_agent_agora_stream.c"
)

set(COMPONENT_ADD_INCLUDEDIRS
    "./qm_config"
    "./agent"
    "./agent/private"
    "./cloud"
    "./peripheral"
    "./voice"
    "${QDEV}/components/qm_agent/include"
    "${QDEV}/components/qm_agent/priv_include"
    "${QDEV}/components/qm_agent/src/transport"
    "${AGORA_SDK}/include"
    "${QDEV}/components/qm_websocket_client/include"
    "${QDEV}/components/qm_websocket_client/src"
    "${QDEV}/components/qm_time/include"
    "${QDEV}/utility/uuid"
    "${QDEV}/utility/sprintf"
    "${QDEV}/include"
    "${QDEV}/include/qmos"
    "${QDEV}/include/common"
    "${QDEV}/include/hal/soc"
    "${QDEV}/include/hal/wifi"
    "${QDEV}/include/hal/net"
    "${QDEV}/include/hal/ble"
    "${QDEV}/components/qm_audio/include"
    "${QDEV}/components/qm_audio/priv_include"
    "${QDEV}/components/qm_event/include"
    "${QDEV}/components/qm_provision/include"
    "${QDEV}/components/qm_cloud/include"
    "${QDEV}/components/qm_cloud/api"
    "${QDEV}/components/qm_cloud/common"
    "${QDEV}/components/qm_cloud/config"
    "${QDEV}/components/qm_cloud/dev"
    "${QDEV}/components/qm_cloud/dynreg"
    "${QDEV}/components/qm_cloud/mqtt"
    "${QDEV}/components/qm_cloud/pub"
    "${QDEV}/components/qm_cloud/shadow"
    "${QDEV}/components/qm_cloud/priv_include"
    "${QDEV}/components/qm_spec/include"
    "${QDEV}/components/qm_ble"
    "${QDEV}/components/qm_ble/api"
    "${QDEV}/components/qm_ble/core"
    "${QDEV}/components/qm_ble/include"
    "${QDEV}/components/qm_ble/hal/include"
    "${QDEV}/components/qm_db/include"
    "${QDEV}/components/qm_db/hal"
    "${QDEV}/components/wifi_manager/include"
    "${QDEV}/components/qm_http_client/include"
    "${QDEV}/components/http"
    "${QDEV}/components/network"
    "${QDEV}/components/mqtt"
    "${QDEV}/components/mqtt/MQTTPacket"
    "${QDEV}/components/backoff/include"
    "${QDEV}/components/cjson"
    "${QDEV}/drivers/VB701N/include"
    "${QDEV}/drivers/VB701N/proto"
    "${QDEV}/utility/string"
    "${QDEV}/utility/list"
    "${QDEV}/utility/timer"
    "${QDEV}/utility/json_parser"
    "${QDEV}/utility/hmac"
    "${QDEV}/utility/base64"
    "${QDEV}/utility/md5"
    "${QDEV}/utility/sha256"
    "${QDEV}/utility/sha1"
    "${QDEV}/utility/aes"
    "${QDEV}/components/qm_time/include"
)

set(COMPONENT_REQUIRES
    esp_driver_uart esp_driver_gpio esp_timer driver vfs
    esp_wifi esp_netif esp_event nvs_flash bt mbedtls esp-tls
    esp-idf-lib__mpu6050
    espressif__led_strip
)

if(VOICE_BACKEND_AGORA)
    list(APPEND COMPONENT_REQUIRES pthread 78__esp-opus)
endif()

register_component()

if(VOICE_BACKEND_AGORA)
    target_link_libraries(${COMPONENT_LIB} PUBLIC
        "${AGORA_SDK}/libs/librtsa.a"
        "${AGORA_SDK}/libs/libahpl.a"
        "${AGORA_SDK}/libs/libagora-cjson.a"
    )
endif()
