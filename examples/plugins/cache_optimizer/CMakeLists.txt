cmake_minimum_required(VERSION 3.28)
project(CacheOptimizerPlugin)

find_package(Qt6 REQUIRED COMPONENTS Core)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(PLUGIN_SOURCES
    CacheOptimizerPlugin.cpp
    CacheOptimizerPlugin.h
)

add_library(cache_optimizer SHARED ${PLUGIN_SOURCES})
target_link_libraries(cache_optimizer PRIVATE Qt6::Core)

set_target_properties(cache_optimizer PROPERTIES
    OUTPUT_NAME "cache_optimizer"
    PREFIX ""
)

add_custom_command(TARGET cache_optimizer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/cache_optimizer.json
        $<TARGET_FILE_DIR:cache_optimizer>/cache_optimizer.json
)

install(TARGETS cache_optimizer
    RUNTIME DESTINATION bin/plugins
    LIBRARY DESTINATION bin/plugins
)

install(FILES cache_optimizer.json DESTINATION bin/plugins)
