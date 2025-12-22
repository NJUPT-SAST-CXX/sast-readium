cmake_minimum_required(VERSION 3.28)
project(DocumentStatisticsPlugin)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(PLUGIN_SOURCES
    DocumentStatisticsPlugin.cpp
    DocumentStatisticsPlugin.h
)

add_library(document_statistics SHARED ${PLUGIN_SOURCES})
target_link_libraries(document_statistics PRIVATE Qt6::Core Qt6::Widgets)

set_target_properties(document_statistics PROPERTIES
    OUTPUT_NAME "document_statistics"
    PREFIX ""
)

add_custom_command(TARGET document_statistics POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/document_statistics.json
        $<TARGET_FILE_DIR:document_statistics>/document_statistics.json
)

install(TARGETS document_statistics
    RUNTIME DESTINATION bin/plugins
    LIBRARY DESTINATION bin/plugins
)

install(FILES document_statistics.json DESTINATION bin/plugins)
