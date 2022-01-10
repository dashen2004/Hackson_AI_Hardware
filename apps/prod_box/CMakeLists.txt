# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/apps/${CMAKE_PROJECT_NAME}/drivers
                         $ENV{IDF_PATH}/apps/${CMAKE_PROJECT_NAME}/qumi/build/esp32x
                         $ENV{IDF_PATH}/apps/${CMAKE_PROJECT_NAME}/app
   )

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(prod_box)

