#!/bin/bash
# windows需要有python，且安装了esptool，安装命令pip install esptool

current_datetime=$(date +'%Y%m%d_%H_%M_%S')
echo $current_datetime

CHIP_TYPE="esp32s3"

ESP_TOOL_PATH="//mnt/d/work/$CHIP_TYPE/bin/"
ESP_TOOL_WIN_PATH="D:/work/$CHIP_TYPE/bin/"

BOOTLOADER_PATH="build/bootloader/bootloader.bin"
PARTITION_TABLE_PATH="build/partition_table/partition-table.bin"
OTA_DATA_INITIAL_PATH="build/ota_data_initial.bin"
PHY_INIT_DATA_PATH="build/phy_init_data.bin"
VOLC_RTC_PATH="build/volc_rtc.bin"
SRMODELS_PATH="build/srmodels/srmodels.bin"
SPIFFS_DATA_PATH="build/spiffs_data.bin"
LVGL_PATH="build/lvgl.bin"

VOLC_RTC_MAP_PATH="build/volc_rtc.map"


COMBINE_OUTPUT_FILE="LIGHT.0059.G.101_FW_WB74_$current_datetime.bin"

idf_build() {
if type idf.py >/dev/null 2>&1; then
echo "idf.py exist"
else
. ./../export.sh
fi

idf.py build
}

idf_menuconfig() {
if type idf.py >/dev/null 2>&1; then
echo "idf.py exist"
else
. ./../export.sh
fi

idf.py menuconfig
}

wsl_file_to_windwos() {
echo
echo "文件路径$ESP_TOOL_WIN_PATH"
if [ -d "$ESP_TOOL_PATH" ]; then
rm -r $ESP_TOOL_PATH
fi

mkdir -p $ESP_TOOL_PATH

cp $BOOTLOADER_PATH $ESP_TOOL_PATH
cp $PARTITION_TABLE_PATH $ESP_TOOL_PATH
cp $OTA_DATA_INITIAL_PATH $ESP_TOOL_PATH
cp $PHY_INIT_DATA_PATH $ESP_TOOL_PATH
cp $VOLC_RTC_PATH $ESP_TOOL_PATH
cp $SRMODELS_PATH $ESP_TOOL_PATH
cp $SPIFFS_DATA_PATH $ESP_TOOL_PATH
cp $LVGL_PATH $ESP_TOOL_PATH
cp $VOLC_RTC_MAP_PATH $ESP_TOOL_PATH

echo
echo "copy $BOOTLOADER_PATH           to $ESP_TOOL_WIN_PATH"
echo "copy $PARTITION_TABLE_PATH      to $ESP_TOOL_WIN_PATH"
echo "copy $OTA_DATA_INITIAL_PATH     to $ESP_TOOL_WIN_PATH"
echo "copy $PHY_INIT_DATA_PATH        to $ESP_TOOL_WIN_PATH"
echo "copy $VOLC_RTC_PATH             to $ESP_TOOL_WIN_PATH"
echo "copy $SRMODELS_PATH             to $ESP_TOOL_WIN_PATH"
echo "copy $SPIFFS_DATA_PATH          to $ESP_TOOL_WIN_PATH"
echo "copy $LVGL_PATH                 to $ESP_TOOL_WIN_PATH"
echo "copy $VOLC_RTC_MAP_PATH         to $ESP_TOOL_WIN_PATH"
}

esp_merge_bin() {
if cmd.exe /c "where python" > /dev/null 2>&1; then
echo "Python exists"
else
echo "please install python"
return
fi

if cmd.exe /c "where esptool" > /dev/null 2>&1; then
echo "esptool exist"
else
cmd.exe /c "pip install esptool"
fi

cmd.exe /C "esptool --chip $CHIP_TYPE merge-bin \
-o $ESP_TOOL_WIN_PATH$COMBINE_OUTPUT_FILE \
--flash-mode dio \
--flash-freq 80m \
0x0 $ESP_TOOL_WIN_PATH"bootloader.bin" \
0x10000 $ESP_TOOL_WIN_PATH"partition-table.bin" \
0x1d000 $ESP_TOOL_WIN_PATH"ota_data_initial.bin" \
0x1f000 $ESP_TOOL_WIN_PATH"phy_init_data.bin" \
0x20000 $ESP_TOOL_WIN_PATH"volc_rtc.bin" \
0x800000 $ESP_TOOL_WIN_PATH"srmodels.bin" \
0xbf0000 $ESP_TOOL_WIN_PATH"spiffs_data.bin" \
0xcf0000 $ESP_TOOL_WIN_PATH"lvgl.bin""
}

esp_factory_flash() {
echo "esp_factory_flash port: $PORT"
cmd.exe /C "esptool --chip $CHIP_TYPE -b 1152000 -p $1 write-flash --flash-mode dio --flash-size detect --flash-freq 80m \
0x0 $ESP_TOOL_WIN_PATH$COMBINE_OUTPUT_FILE"
}

esp_app_flash() {
echo "esp_app_flash port: $PORT"
cmd.exe /C "esptool --chip $CHIP_TYPE -b 1152000 -p $1 write-flash --flash-mode dio --flash-size detect --flash-freq 80m \
0x20000 $ESP_TOOL_WIN_PATH"volc_rtc.bin""
}

check_cmd() {
if [ -z "$1" ]; then
    idf_build
    wsl_file_to_windwos
    esp_merge_bin
    return
fi
PORT="com3"
if [ -n "$2" ]; then
    PORT=$2
fi
case $1 in
"build"|"-b")
    idf_build
    ;; 
"flash"|"-f")
    idf_build
    wsl_file_to_windwos
    esp_merge_bin
    esp_factory_flash "$PORT"
    ;;
"app_flash"|"-af")
    idf_build
    wsl_file_to_windwos
    esp_merge_bin
    esp_app_flash "$PORT"
    ;;
"menuconfig"|"-mc")
    idf_menuconfig
    ;;
"help"|"-h")
    echo "Usage: . ./run.sh [cmd] [param]"
    echo "cmd:"
    echo "    (default)                 build project, cp file to windows, merge bin to factory firmware"
    echo "    build, -b                 build project"
    echo "    flash, -f                 flash factory firmware"
    echo "    app_flash, -af            flash app firmware"
    echo "    menuconfig, -mc           idf menuconfig"
    echo "    help, -h                  show cmd"
    echo "param:"
    echo "    flash or app_flash can select port, defult use com3"
    ;;
*)
    echo "Usage: . ./run.sh help"
    ;;
esac
}

check_cmd $1 $2
