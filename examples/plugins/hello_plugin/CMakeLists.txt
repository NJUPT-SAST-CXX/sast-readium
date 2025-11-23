cmake_minimum_required(VERSION 3.28)
project(HelloPlugin)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable automatic MOC
set(CMAKE_AUTOMOC ON)

# Plugin sources
set(PLUGIN_SOURCES
    HelloPlugin.cpp
    HelloPlugin.h
)

# Create plugin library
add_library(hello_plugin SHARED ${PLUGIN_SOURCES})

# Link Qt6
target_link_libraries(hello_plugin PRIVATE
    Qt6::Core
    Qt6::Widgets
)

# Set plugin properties
set_target_properties(hello_plugin PROPERTIES
    OUTPUT_NAME "hello_plugin"
    PREFIX ""
)

# Copy plugin metadata JSON file to output directory
add_custom_command(TARGET hello_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/hello_plugin.json
        $<TARGET_FILE_DIR:hello_plugin>/hello_plugin.json
)

# Installation
# Install into bin/plugins so that the main application can discover the plugin
install(TARGETS hello_plugin
    RUNTIME DESTINATION bin/plugins
    LIBRARY DESTINATION bin/plugins
)

install(FILES hello_plugin.json
    DESTINATION bin/plugins
)
