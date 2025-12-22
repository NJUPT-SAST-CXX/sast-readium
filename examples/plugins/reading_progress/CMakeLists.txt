cmake_minimum_required(VERSION 3.28)
project(ReadingProgressPlugin)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(PLUGIN_SOURCES
    ReadingProgressPlugin.cpp
    ReadingProgressPlugin.h
)

add_library(reading_progress SHARED ${PLUGIN_SOURCES})
target_link_libraries(reading_progress PRIVATE Qt6::Core Qt6::Widgets)

set_target_properties(reading_progress PROPERTIES
    OUTPUT_NAME "reading_progress"
    PREFIX ""
)

add_custom_command(TARGET reading_progress POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/reading_progress.json
        $<TARGET_FILE_DIR:reading_progress>/reading_progress.json
)

install(TARGETS reading_progress
    RUNTIME DESTINATION bin/plugins
    LIBRARY DESTINATION bin/plugins
)

install(FILES reading_progress.json DESTINATION bin/plugins)
