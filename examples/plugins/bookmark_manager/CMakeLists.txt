cmake_minimum_required(VERSION 3.28)
project(BookmarkManagerPlugin)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(PLUGIN_SOURCES
    BookmarkManagerPlugin.cpp
    BookmarkManagerPlugin.h
)

add_library(bookmark_manager SHARED ${PLUGIN_SOURCES})
target_link_libraries(bookmark_manager PRIVATE Qt6::Core Qt6::Widgets)

set_target_properties(bookmark_manager PROPERTIES
    OUTPUT_NAME "bookmark_manager"
    PREFIX ""
)

add_custom_command(TARGET bookmark_manager POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/bookmark_manager.json
        $<TARGET_FILE_DIR:bookmark_manager>/bookmark_manager.json
)

install(TARGETS bookmark_manager
    RUNTIME DESTINATION bin/plugins
    LIBRARY DESTINATION bin/plugins
)

install(FILES bookmark_manager.json DESTINATION bin/plugins)
