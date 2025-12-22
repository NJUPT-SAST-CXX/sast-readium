cmake_minimum_required(VERSION 3.28)
project(SmartSearchPlugin)

find_package(Qt6 REQUIRED COMPONENTS Core)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(PLUGIN_SOURCES
    SmartSearchPlugin.cpp
    SmartSearchPlugin.h
)

add_library(smart_search SHARED ${PLUGIN_SOURCES})

target_link_libraries(smart_search PRIVATE Qt6::Core)

set_target_properties(smart_search PROPERTIES
    OUTPUT_NAME "smart_search"
    PREFIX ""
)

add_custom_command(TARGET smart_search POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/smart_search.json
        $<TARGET_FILE_DIR:smart_search>/smart_search.json
)

install(TARGETS smart_search
    RUNTIME DESTINATION bin/plugins
    LIBRARY DESTINATION bin/plugins
)

install(FILES smart_search.json DESTINATION bin/plugins)
