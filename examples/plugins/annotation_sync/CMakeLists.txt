cmake_minimum_required(VERSION 3.28)
project(AnnotationSyncPlugin)

find_package(Qt6 REQUIRED COMPONENTS Core Gui)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(PLUGIN_SOURCES
    AnnotationSyncPlugin.cpp
    AnnotationSyncPlugin.h
)

add_library(annotation_sync SHARED ${PLUGIN_SOURCES})
target_link_libraries(annotation_sync PRIVATE Qt6::Core Qt6::Gui)

set_target_properties(annotation_sync PROPERTIES
    OUTPUT_NAME "annotation_sync"
    PREFIX ""
)

add_custom_command(TARGET annotation_sync POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/annotation_sync.json
        $<TARGET_FILE_DIR:annotation_sync>/annotation_sync.json
)

install(TARGETS annotation_sync
    RUNTIME DESTINATION bin/plugins
    LIBRARY DESTINATION bin/plugins
)

install(FILES annotation_sync.json DESTINATION bin/plugins)
