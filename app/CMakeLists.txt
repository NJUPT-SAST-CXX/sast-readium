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

    # Core UI components
    ui/core/MenuBar.cpp
    ui/core/SideBar.cpp
    ui/core/RightSideBar.cpp
    ui/core/StatusBar.cpp
    ui/core/ToolBar.cpp
    ui/core/ViewWidget.cpp

    # Viewer components
    ui/viewer/PDFViewer.cpp
    ui/viewer/PDFOutlineWidget.cpp
    ui/viewer/PDFViewerEnhancements.cpp
    ui/viewer/PDFAnimations.cpp
    ui/viewer/QGraphicsPDFViewer.cpp

    # UI Widgets
    ui/widgets/DocumentTabWidget.cpp
    ui/widgets/SearchWidget.cpp
    ui/widgets/BookmarkWidget.cpp
    ui/widgets/AnnotationToolbar.cpp
    ui/widgets/WelcomeWidget.cpp
    ui/widgets/RecentFileListWidget.cpp
    ui/widgets/DebugLogPanel.cpp

    # Dialog components
    ui/dialogs/DocumentComparison.cpp
    ui/dialogs/DocumentMetadataDialog.cpp

    # Thumbnail system
    ui/thumbnail/ThumbnailWidget.cpp
    ui/thumbnail/ThumbnailGenerator.cpp
    ui/thumbnail/ThumbnailListView.cpp
    ui/thumbnail/ThumbnailContextMenu.cpp

    # UI Manager components
    ui/managers/WelcomeScreenManager.cpp

    # Core Manager components
    managers/StyleManager.cpp
    managers/FileTypeIconManager.cpp
    managers/I18nManager.cpp

    # Viewer components (continued)
    ui/viewer/PDFPrerenderer.cpp

    # Cache management
    cache/PDFCacheManager.cpp

    # Utility components (temporarily removed due to compilation issues)
    utils/PDFUtilities.cpp
    utils/DocumentAnalyzer.cpp

    # Manager components
    managers/RecentFilesManager.cpp

    # Other components
    plugin/PluginManager.cpp
    factory/WidgetFactory.cpp
    command/Commands.cpp

    # Controllers
    controller/DocumentController.cpp
    controller/PageController.cpp

    # Models
    model/DocumentModel.cpp
    model/PageModel.cpp
    model/PDFOutlineModel.cpp
    model/AsyncDocumentLoader.cpp
    model/SearchModel.cpp
    model/BookmarkModel.cpp
    model/AnnotationModel.cpp
    model/RenderModel.cpp
    model/ThumbnailModel.cpp

    # Delegates
    delegate/PageNavigationDelegate.cpp
    delegate/ThumbnailDelegate.cpp

    # Views
    view/Views.cpp

    # Logging and Error Handling
    logging/Logger.cpp
    logging/QtSpdlogBridge.cpp
    logging/LoggingManager.cpp
    logging/LoggingMacros.cpp
    logging/LoggingConfig.cpp
    logging/SimpleLogging.cpp
    utils/ErrorHandling.cpp
    utils/ErrorRecovery.cpp

    # Tests (optional, can be excluded in release builds)
    # tests/test_logging.cpp  # Commented out - file doesn't exist yet
)
target_include_directories(
    app
    PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(
    app
    PRIVATE
    Qt::Core Qt::Gui Qt::Widgets Qt::Svg
    PkgConfig::POPPLER_QT6
    spdlog::spdlog
)

# Add QGraphics PDF support define if enabled
if(ENABLE_QGRAPHICS_PDF_SUPPORT)
    target_compile_definitions(app PRIVATE ENABLE_QGRAPHICS_PDF_SUPPORT)
endif()

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
