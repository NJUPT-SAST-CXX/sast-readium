cmake_minimum_required(VERSION 3.28)
project(ThemeProviderPlugin)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(PLUGIN_SOURCES
    ThemeProviderPlugin.cpp
    ThemeProviderPlugin.h
)

add_library(theme_provider SHARED ${PLUGIN_SOURCES})
target_link_libraries(theme_provider PRIVATE Qt6::Core Qt6::Widgets)

set_target_properties(theme_provider PROPERTIES
    OUTPUT_NAME "theme_provider"
    PREFIX ""
)

add_custom_command(TARGET theme_provider POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/theme_provider.json
        $<TARGET_FILE_DIR:theme_provider>/theme_provider.json
)

install(TARGETS theme_provider
    RUNTIME DESTINATION bin/plugins
    LIBRARY DESTINATION bin/plugins
)

install(FILES theme_provider.json DESTINATION bin/plugins)
