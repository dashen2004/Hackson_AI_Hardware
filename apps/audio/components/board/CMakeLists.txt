set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES esp_peripherals audio_sal audio_hal esp_dispatcher display_service)

list(APPEND COMPONENT_ADD_INCLUDEDIRS 
    # "./dual_mic_board")
    "./single_mic_board")

set(COMPONENT_SRCS 
    # "./dual_mic_board/board.c"
    # "./dual_mic_board/board_pins_config.c")
    "./single_mic_board/board.c"
    "./single_mic_board/board_pins_config.c")
    
register_component()

add_definitions(-DENBALE_AUDIO_STREAM_DUAL_MIC)

IF (IDF_VERSION_MAJOR GREATER 3)
idf_component_get_property(audio_board_lib audio_board COMPONENT_LIB)
set_property(TARGET ${audio_board_lib} APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${COMPONENT_LIB})

ELSEIF (IDF_VERSION_MAJOR EQUAL 3)
set_property(TARGET idf_component_audio_board APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES  $<TARGET_PROPERTY:${COMPONENT_TARGET},INTERFACE_INCLUDE_DIRECTORIES>)

ENDIF (IDF_VERSION_MAJOR GREATER 3)