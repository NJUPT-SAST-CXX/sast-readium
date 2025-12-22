cmake_minimum_required(VERSION 3.28)
project(PageNavigatorPlugin)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(PLUGIN_SOURCES
    PageNavigatorPlugin.cpp
    PageNavigatorPlugin.h
)

add_library(page_navigator SHARED ${PLUGIN_SOURCES})
target_link_libraries(page_navigator PRIVATE Qt6::Core Qt6::Widgets)

set_target_properties(page_navigator PROPERTIES
    OUTPUT_NAME "page_navigator"
    PREFIX ""
)

add_custom_command(TARGET page_navigator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/page_navigator.json
        $<TARGET_FILE_DIR:page_navigator>/page_navigator.json
)

install(TARGETS page_navigator
    RUNTIME DESTINATION bin/plugins
    LIBRARY DESTINATION bin/plugins
)

install(FILES page_navigator.json DESTINATION bin/plugins)
