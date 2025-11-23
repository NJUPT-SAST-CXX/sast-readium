cmake_minimum_required(VERSION 3.28)
project(MetadataExtractorPlugin)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable automatic MOC
set(CMAKE_AUTOMOC ON)

# Plugin sources
set(PLUGIN_SOURCES
    MetadataExtractorPlugin.cpp
    MetadataExtractorPlugin.h
)

# Create plugin library
add_library(metadata_extractor_plugin SHARED ${PLUGIN_SOURCES})

# Add include directories so we can include app/plugin headers
target_include_directories(metadata_extractor_plugin PRIVATE
    ${CMAKE_SOURCE_DIR}/app
)

# Link Qt6
target_link_libraries(metadata_extractor_plugin PRIVATE
    Qt6::Core
)

# Set plugin properties
set_target_properties(metadata_extractor_plugin PROPERTIES
    OUTPUT_NAME "metadata_extractor"
    PREFIX ""
)

# Installation: install into bin/plugins so the main app can discover it
install(TARGETS metadata_extractor_plugin
    RUNTIME DESTINATION bin/plugins
    LIBRARY DESTINATION bin/plugins
)

install(FILES metadata_extractor.json
    DESTINATION bin/plugins
)
