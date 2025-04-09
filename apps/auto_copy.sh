#!/bin/bash
idf.py build
cp ./build/*.bin //mnt/c/Users/25456/Desktop/esp32/esp32s3/test
cp ./build/bootloader/bootloader.bin //mnt/c/Users/25456/Desktop/esp32/esp32s3/test
cp ./build/partition_table/*.bin //mnt/c/Users/25456/Desktop/esp32/esp32s3/test
cp ./build/srmodels/*.bin //mnt/c/Users/25456/Desktop/esp32/esp32s3/test


# cp ./build/*.bin //mnt/c/Users/25456/Desktop/esp32/网关

echo "---copy complete---"