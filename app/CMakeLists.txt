project(app LANGUAGES CXX VERSION 0.1.0.0 DESCRIPTION "SAST Readium")

configure_file(config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

file(GLOB_RECURSE APP_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

file(GLOB UIs "${CMAKE_CURRENT_SOURCE_DIR}/*.ui")

if (WIN32)
    add_executable(app app.rc)
    set_target_properties(app PROPERTIES
        WIN32_EXECUTABLE $<IF:$<CONFIG:Release>,ON,OFF>
    )
else ()
    add_executable(app)
endif()

# 若无下面这个设置，则 exe 文件会生成到 build/Debug 中，而 qm 文件会生成到 build/Debug/app 目录下
set_target_properties(
    app
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

qt_add_translations(
    app
    SOURCES ${UIs}
    TS_FILE_DIR ${CMAKE_SOURCE_DIR}/app/i18n
    TS_FILE_BASE app
)

target_sources(
    app
    PRIVATE
    MainWindow.cpp
    main.cpp
    ui/MenuBar.cpp
    ui/SideBar.cpp
    ui/StatusBar.cpp
    ui/ToolBar.cpp
    ui/ViewWidget.cpp

    factory/WidgetFactory.cpp
    
    command/Commands.cpp

    controller/DocumentController.cpp
    controller/PageController.cpp

    model/DocumentModel.cpp
    model/PageModel.cpp
    model/RenderModel.cpp

    delegate/PageNavigationDelegate.cpp
    
    view/Views.cpp
)
target_include_directories(
    app
    PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(
    app
    PRIVATE
    Qt::Core Qt::Gui Qt::Widgets Qt::Svg
    controls
    PkgConfig::POPPLER_QT6
)

# fix the "qt.qpa.plugin: Could not find the Qt platform plugin 'windows' in ''" issue
if (WIN32)
    add_custom_command(
        TARGET app
        POST_BUILD
        COMMAND Qt6::windeployqt
        ARGS $<TARGET_FILE:app>
    )
endif()

add_dependencies(app COPYING_ASSETS)
add_custom_target(COPYING_ASSETS DEPENDS ${CMAKE_BINARY_DIR}/COPYING_ASSETS_DUMMY)
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/COPYING_ASSETS_DUMMY
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets/styles $<TARGET_FILE_DIR:app>/styles
)
