cmake_minimum_required(VERSION 3.28)
project(TTSEnhancerPlugin)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets TextToSpeech)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(PLUGIN_SOURCES
    TTSEnhancerPlugin.cpp
    TTSEnhancerPlugin.h
)

add_library(tts_enhancer SHARED ${PLUGIN_SOURCES})
target_link_libraries(tts_enhancer PRIVATE Qt6::Core Qt6::Widgets Qt6::TextToSpeech)

set_target_properties(tts_enhancer PROPERTIES
    OUTPUT_NAME "tts_enhancer"
    PREFIX ""
)

add_custom_command(TARGET tts_enhancer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/tts_enhancer.json
        $<TARGET_FILE_DIR:tts_enhancer>/tts_enhancer.json
)

install(TARGETS tts_enhancer
    RUNTIME DESTINATION bin/plugins
    LIBRARY DESTINATION bin/plugins
)

install(FILES tts_enhancer.json DESTINATION bin/plugins)
