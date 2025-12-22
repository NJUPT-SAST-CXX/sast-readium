cmake_minimum_required(VERSION 3.28)
project(KeyboardShortcutsPlugin)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(PLUGIN_SOURCES
    KeyboardShortcutsPlugin.cpp
    KeyboardShortcutsPlugin.h
)

add_library(keyboard_shortcuts SHARED ${PLUGIN_SOURCES})
target_link_libraries(keyboard_shortcuts PRIVATE Qt6::Core Qt6::Widgets)

set_target_properties(keyboard_shortcuts PROPERTIES
    OUTPUT_NAME "keyboard_shortcuts"
    PREFIX ""
)

add_custom_command(TARGET keyboard_shortcuts POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/keyboard_shortcuts.json
        $<TARGET_FILE_DIR:keyboard_shortcuts>/keyboard_shortcuts.json
)

install(TARGETS keyboard_shortcuts
    RUNTIME DESTINATION bin/plugins
    LIBRARY DESTINATION bin/plugins
)

install(FILES keyboard_shortcuts.json DESTINATION bin/plugins)
