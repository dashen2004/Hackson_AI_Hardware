

file(GLOB srcs "*.c") 

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "."
                       REQUIRES qumi main
                       )
