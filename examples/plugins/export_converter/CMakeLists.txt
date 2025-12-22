cmake_minimum_required(VERSION 3.28)
project(ExportConverterPlugin)

find_package(Qt6 REQUIRED COMPONENTS Core)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(PLUGIN_SOURCES
    ExportConverterPlugin.cpp
    ExportConverterPlugin.h
)

add_library(export_converter SHARED ${PLUGIN_SOURCES})
target_link_libraries(export_converter PRIVATE Qt6::Core)

set_target_properties(export_converter PROPERTIES
    OUTPUT_NAME "export_converter"
    PREFIX ""
)

add_custom_command(TARGET export_converter POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/export_converter.json
        $<TARGET_FILE_DIR:export_converter>/export_converter.json
)

install(TARGETS export_converter
    RUNTIME DESTINATION bin/plugins
    LIBRARY DESTINATION bin/plugins
)

install(FILES export_converter.json DESTINATION bin/plugins)
