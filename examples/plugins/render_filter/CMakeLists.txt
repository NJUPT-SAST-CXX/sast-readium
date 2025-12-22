cmake_minimum_required(VERSION 3.28)
project(RenderFilterPlugin)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable automatic MOC
set(CMAKE_AUTOMOC ON)

# Plugin sources
set(PLUGIN_SOURCES
    RenderFilterPlugin.cpp
    RenderFilterPlugin.h
)

# Create plugin library
add_library(render_filter SHARED ${PLUGIN_SOURCES})

# Link Qt6
target_link_libraries(render_filter PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Set plugin properties
set_target_properties(render_filter PROPERTIES
    OUTPUT_NAME "render_filter"
    PREFIX ""
)

# Copy plugin metadata JSON file to output directory
add_custom_command(TARGET render_filter POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/render_filter.json
        $<TARGET_FILE_DIR:render_filter>/render_filter.json
)

# Installation
install(TARGETS render_filter
    RUNTIME DESTINATION bin/plugins
    LIBRARY DESTINATION bin/plugins
)

install(FILES render_filter.json
    DESTINATION bin/plugins
)
