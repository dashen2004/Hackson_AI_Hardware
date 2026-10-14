cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

file(READ "${CMAKE_CURRENT_LIST_DIR}/main/qm_config/qm_config.h" QM_CONFIG_H)
if(QM_CONFIG_H MATCHES "#define[ \t]+CONFIG_VOICE_BACKEND_AGORA[ \t]+1")
    set(EXTRA_COMPONENT_DIRS
        "${CMAKE_CURRENT_LIST_DIR}/../../esp32_arrow/apps/managed_components/78__esp-opus")
endif()

project(vb701n)
