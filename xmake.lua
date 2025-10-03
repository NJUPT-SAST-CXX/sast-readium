-- xmake.lua for SAST Readium
-- A Qt6-based PDF reader application with comprehensive build support

-- Set minimum xmake version
set_xmakever("2.8.0")

-- Project metadata
set_project("sast-readium")
set_version("0.1.0.0")
set_description("SAST Readium - A Qt6-based PDF reader application")

-- Language and standards
set_languages("cxx20")
set_warnings("all")

-- Build modes
add_rules("mode.debug", "mode.release")

-- Package requirements (only non-Qt packages)
add_requires("pkgconfig::poppler-qt6")
add_requires("spdlog")

-- Build options
option("toolchain")
    set_default("auto")
    set_description("Select toolchain (auto, mingw, msvc, clang, gcc)")
    set_values("auto", "mingw", "msvc", "clang", "gcc")
    set_showmenu(true)
option_end()

option("qt_path")
    set_default("")
    set_description("Custom Qt installation path")
    set_showmenu(true)
option_end()

option("enable_clangd")
    set_default(true)
    set_description("Enable clangd configuration generation")
    set_showmenu(true)
option_end()

option("enable_qgraphics_pdf")
    set_default(false)
    set_description("Enable QGraphics-based PDF rendering support")
    set_showmenu(true)
option_end()

option("enable_tests")
    set_default(false)
    set_description("Enable building tests")
    set_showmenu(true)
option_end()



-- Platform and toolchain detection functions
function is_msys2()
    return is_plat("windows") and os.getenv("MSYSTEM")
end

function get_default_toolchain()
    if is_plat("windows") then
        if is_msys2() then
            return "mingw"
        else
            return "msvc"
        end
    elseif is_plat("linux") then
        return "gcc"
    elseif is_plat("macosx") then
        return "clang"
    else
        return "gcc"
    end
end

function configure_toolchain()
    local toolchain = get_config("toolchain")
    if toolchain == "auto" then
        toolchain = get_default_toolchain()
    end


    if toolchain == "mingw" then
        set_toolchains("mingw")
    elseif toolchain == "msvc" then
        set_toolchains("msvc")
    elseif toolchain == "clang" then
        set_toolchains("clang")
    elseif toolchain == "gcc" then
        set_toolchains("gcc")
    end

    return toolchain
end

function find_qt_installation()
    local custom_qt = get_config("qt_path")
    if custom_qt and custom_qt ~= "" then
        return custom_qt
    end

    if is_plat("windows") then
        -- Try common Qt installation paths on Windows
        local qt_paths = {
            "D:/msys64/mingw64",  -- MSYS2
            "C:/Qt/6.5.0/mingw_64",  -- Qt installer with MinGW
            "C:/Qt/6.5.0/msvc2022_64",  -- Qt installer with MSVC
            "C:/Qt/Tools/mingw1120_64",  -- Qt Tools MinGW
        }

        for _, qt_path in ipairs(qt_paths) do
            if os.isdir(qt_path .. "/include/qt6") then
                return qt_path
            end
        end
    elseif is_plat("linux") then
        -- Try common Qt installation paths on Linux
        local qt_paths = {
            "/usr",
            "/usr/local",
            "/opt/qt6",
            os.getenv("HOME") .. "/Qt/6.5.0/gcc_64"
        }

        for _, qt_path in ipairs(qt_paths) do
            if os.isdir(qt_path .. "/include/qt6") or os.isdir(qt_path .. "/include/QtCore") then
                return qt_path
            end
        end
    elseif is_plat("macosx") then
        -- Try common Qt installation paths on macOS
        local qt_paths = {
            "/usr/local",
            "/opt/homebrew",
            "/opt/qt6",
            os.getenv("HOME") .. "/Qt/6.5.0/macos"
        }

        for _, qt_path in ipairs(qt_paths) do
            if os.isdir(qt_path .. "/include/qt6") or os.isdir(qt_path .. "/include/QtCore") then
                return qt_path
            end
        end
    end

    return nil
end

-- Configure toolchain before defining targets
configure_toolchain()

-- Main application target
target("sast-readium")
    set_kind("binary")

    -- Add Qt rules for MOC processing
    add_rules("qt.moc")

    -- Explicitly add headers that need MOC processing
    add_files("app/MainWindow.h")
    add_files("app/model/DocumentModel.h")
    add_files("app/model/PageModel.h")
    add_files("app/model/RenderModel.h")
    add_files("app/model/PDFOutlineModel.h")
    add_files("app/controller/DocumentController.h")
    add_files("app/controller/PageController.h")
    add_files("app/managers/StyleManager.h")
    add_files("app/managers/FileTypeIconManager.h")
    add_files("app/managers/RecentFilesManager.h")
    add_files("app/ui/viewer/PDFViewer.h")
    add_files("app/ui/viewer/PDFOutlineWidget.h")
    add_files("app/ui/widgets/DocumentTabWidget.h")
    add_files("app/ui/widgets/SearchWidget.h")
    add_files("app/ui/widgets/BookmarkWidget.h")
    add_files("app/ui/widgets/AnnotationToolbar.h")
    add_files("app/ui/thumbnail/ThumbnailModel.h")
    add_files("app/ui/thumbnail/ThumbnailDelegate.h")
    add_files("app/ui/thumbnail/ThumbnailListView.h")
    add_files("app/ui/thumbnail/ThumbnailGenerator.h")
    add_files("app/model/AsyncDocumentLoader.h")
    add_files("app/ui/dialogs/DocumentMetadataDialog.h")
    add_files("app/ui/dialogs/DocumentComparison.h")
    add_files("app/ui/core/ViewWidget.h")
    add_files("app/ui/core/StatusBar.h")
    add_files("app/ui/core/SideBar.h")
    add_files("app/ui/core/MenuBar.h")
    add_files("app/ui/core/ToolBar.h")
    add_files("app/factory/WidgetFactory.h")
    add_files("app/model/SearchModel.h")
    add_files("app/model/BookmarkModel.h")
    add_files("app/model/AnnotationModel.h")
    add_files("app/ui/viewer/PDFPrerenderer.h")
    add_files("app/ui/viewer/PDFAnimations.h")
    add_files("app/ui/viewer/PDFViewerEnhancements.h")
    add_files("app/ui/viewer/QGraphicsPDFViewer.h")
    add_files("app/command/Commands.h")
    add_files("app/cache/PDFCacheManager.h")
    add_files("app/plugin/PluginManager.h")
    add_files("app/delegate/PageNavigationDelegate.h")
    add_files("app/utils/DocumentAnalyzer.h")
    add_files("app/ui/thumbnail/ThumbnailWidget.h")
    add_files("app/ui/thumbnail/ThumbnailContextMenu.h")

    -- Set output directory
    set_targetdir("$(builddir)")

    -- Qt configuration
    local qt_base = find_qt_installation()
    if qt_base then
        print("Found Qt installation at: " .. qt_base)

        -- Configure Qt include directories
        if is_plat("windows") then
            add_includedirs(qt_base .. "/include/qt6")
            add_includedirs(qt_base .. "/include/qt6/QtCore")
            add_includedirs(qt_base .. "/include/qt6/QtGui")
            add_includedirs(qt_base .. "/include/qt6/QtWidgets")
            add_includedirs(qt_base .. "/include/qt6/QtSvg")
            add_includedirs(qt_base .. "/include/qt6/QtConcurrent")

            -- Library directories and links
            add_linkdirs(qt_base .. "/lib")
            add_links("Qt6Core", "Qt6Gui", "Qt6Widgets", "Qt6Svg", "Qt6Concurrent")
        elseif is_plat("linux") then
            if os.isdir(qt_base .. "/include/qt6") then
                add_includedirs(qt_base .. "/include/qt6")
                add_includedirs(qt_base .. "/include/qt6/QtCore")
                add_includedirs(qt_base .. "/include/qt6/QtGui")
                add_includedirs(qt_base .. "/include/qt6/QtWidgets")
                add_includedirs(qt_base .. "/include/qt6/QtSvg")
                add_includedirs(qt_base .. "/include/qt6/QtConcurrent")
            else
                add_includedirs(qt_base .. "/include/QtCore")
                add_includedirs(qt_base .. "/include/QtGui")
                add_includedirs(qt_base .. "/include/QtWidgets")
                add_includedirs(qt_base .. "/include/QtSvg")
                add_includedirs(qt_base .. "/include/QtConcurrent")
            end

            add_linkdirs(qt_base .. "/lib")
            add_links("Qt6Core", "Qt6Gui", "Qt6Widgets", "Qt6Svg", "Qt6Concurrent")
        elseif is_plat("macosx") then
            if os.isdir(qt_base .. "/include/qt6") then
                add_includedirs(qt_base .. "/include/qt6")
                add_includedirs(qt_base .. "/include/qt6/QtCore")
                add_includedirs(qt_base .. "/include/qt6/QtGui")
                add_includedirs(qt_base .. "/include/qt6/QtWidgets")
                add_includedirs(qt_base .. "/include/qt6/QtSvg")
                add_includedirs(qt_base .. "/include/qt6/QtConcurrent")
            else
                add_includedirs(qt_base .. "/include/QtCore")
                add_includedirs(qt_base .. "/include/QtGui")
                add_includedirs(qt_base .. "/include/QtWidgets")
                add_includedirs(qt_base .. "/include/QtSvg")
                add_includedirs(qt_base .. "/include/QtConcurrent")
            end

            add_linkdirs(qt_base .. "/lib")
            add_links("Qt6Core", "Qt6Gui", "Qt6Widgets", "Qt6Svg", "Qt6Concurrent")
        end

        -- Qt defines
        add_defines("QT_CORE_LIB", "QT_GUI_LIB", "QT_WIDGETS_LIB", "QT_SVG_LIB")
        if is_mode("release") then
            add_defines("QT_NO_DEBUG")
        end

        -- QGraphics PDF support
        if has_config("enable_qgraphics_pdf") then
            add_defines("ENABLE_QGRAPHICS_PDF_SUPPORT")
        end
    else
        print("Warning: Qt installation not found. Please specify qt_path option.")
    end

    -- Add poppler-qt6 dependency
    add_packages("pkgconfig::poppler-qt6")

    -- Add spdlog dependency
    add_packages("spdlog")

    -- Generate config.h from template
    before_build(function (target)
        local config_content = [[#pragma once

#define PROJECT_NAME "sast-readium"
#define APP_NAME "SAST Readium - A Qt6-based PDF reader application"
#define PROJECT_VER "0.1.0.0"
#define PROJECT_VER_MAJOR "0"
#define PROJECT_VER_MINOR "1"
#define PROJECT_VER_PATCH "0"
]]
        io.writefile("app/config.h", config_content)
        print("Generated config.h")
    end)

    -- Include directories
    add_includedirs(".", "app", "$(builddir)")
    add_includedirs("app/ui", "app/ui/core", "app/ui/viewer", "app/ui/widgets")
    add_includedirs("app/ui/dialogs", "app/ui/thumbnail", "app/ui/managers")
    add_includedirs("app/managers", "app/model", "app/controller", "app/delegate")
    add_includedirs("app/cache", "app/utils", "app/plugin")
    add_includedirs("app/factory", "app/command")

    -- Compiler settings
    set_languages("cxx20")
    set_warnings("all")

    -- Toolchain-specific compiler flags
    local current_toolchain = get_config("toolchain")
    if current_toolchain == "auto" then
        current_toolchain = get_default_toolchain()
    end

    if current_toolchain == "gcc" or current_toolchain == "mingw" then
        add_cxxflags("-std=c++20")
        if is_mode("debug") then
            add_cxxflags("-g", "-O0")
        else
            add_cxxflags("-O3", "-DNDEBUG")
        end
    elseif current_toolchain == "clang" then
        add_cxxflags("-std=c++20")
        if is_mode("debug") then
            add_cxxflags("-g", "-O0")
        else
            add_cxxflags("-O3", "-DNDEBUG")
        end
    elseif current_toolchain == "msvc" then
        add_cxxflags("/std:c++20")
        if is_mode("debug") then
            add_cxxflags("/Od", "/Zi")
        else
            add_cxxflags("/O2", "/DNDEBUG")
        end
    end

    -- Build mode specific settings
    if is_mode("release") then
        set_optimize("fastest")
        add_defines("NDEBUG")
        set_symbols("hidden")
    else
        set_optimize("none")
        set_symbols("debug")
        add_defines("DEBUG", "_DEBUG")
    end

    -- Platform-specific configurations
    if is_plat("windows") then
        add_defines("WIN32", "_WINDOWS", "UNICODE", "_UNICODE")
        if is_mode("release") then
            set_kind("binary")
        end

        -- Windows RC file
        add_files("app/app.rc")

        -- Windows-specific linker flags
        if current_toolchain == "msvc" then
            add_ldflags("/SUBSYSTEM:WINDOWS")
        end
    elseif is_plat("linux") then
        add_defines("LINUX")
        -- Linux-specific settings
        add_syslinks("pthread", "dl")
    elseif is_plat("macosx") then
        add_defines("MACOS")
        -- macOS-specific settings
        add_frameworks("CoreFoundation", "CoreServices")
    end

    -- Translation files (Qt rules disabled to avoid assertion errors)
    -- add_files("app/i18n/app_en.ts")
    -- add_files("app/i18n/app_zh.ts")
    -- add_rules("qt.ts")
    -- set_values("qt.ts.lupdate", "app/i18n/app_en.ts", "app/i18n/app_zh.ts")

    -- Main source files
    add_files("app/main.cpp")
    add_files("app/MainWindow.cpp")

    -- Core UI components
    add_files("app/ui/core/ViewWidget.cpp")
    add_files("app/ui/core/StatusBar.cpp")
    add_files("app/ui/core/SideBar.cpp")
    add_files("app/ui/core/MenuBar.cpp")
    add_files("app/ui/core/ToolBar.cpp")

    -- Models (essential for linking)
    add_files("app/model/DocumentModel.cpp")
    add_files("app/model/PageModel.cpp")
    add_files("app/model/RenderModel.cpp")
    add_files("app/model/PDFOutlineModel.cpp")
    add_files("app/model/AsyncDocumentLoader.cpp")
    add_files("app/model/SearchModel.cpp")
    add_files("app/model/BookmarkModel.cpp")
    add_files("app/model/AnnotationModel.cpp")
    add_files("app/model/ThumbnailModel.cpp")

    -- Controllers
    add_files("app/controller/DocumentController.cpp")
    add_files("app/controller/PageController.cpp")

    -- Managers
    add_files("app/managers/StyleManager.cpp")
    add_files("app/managers/FileTypeIconManager.cpp")
    add_files("app/managers/RecentFilesManager.cpp")

    -- UI Manager components (removed duplicates)

    -- Cache management
    add_files("app/cache/PDFCacheManager.cpp")

    -- Utilities
    add_files("app/utils/PDFUtilities.cpp")
    add_files("app/utils/DocumentAnalyzer.cpp")

    -- Plugin system
    add_files("app/plugin/PluginManager.cpp")

    -- Factory
    add_files("app/factory/WidgetFactory.cpp")

    -- Command system
    add_files("app/command/Commands.cpp")

    -- Viewer components
    add_files("app/ui/viewer/PDFViewer.cpp")
    add_files("app/ui/viewer/PDFOutlineWidget.cpp")
    add_files("app/ui/viewer/PDFViewerEnhancements.cpp")
    add_files("app/ui/viewer/PDFAnimations.cpp")
    add_files("app/ui/viewer/PDFPrerenderer.cpp")
    add_files("app/ui/viewer/QGraphicsPDFViewer.cpp")

    -- Widget components
    add_files("app/ui/widgets/DocumentTabWidget.cpp")
    add_files("app/ui/widgets/SearchWidget.cpp")
    add_files("app/ui/widgets/BookmarkWidget.cpp")
    add_files("app/ui/widgets/AnnotationToolbar.cpp")

    -- Dialog components
    add_files("app/ui/dialogs/DocumentComparison.cpp")
    add_files("app/ui/dialogs/DocumentMetadataDialog.cpp")

    -- Thumbnail system
    add_files("app/ui/thumbnail/ThumbnailWidget.cpp")

    add_files("app/ui/thumbnail/ThumbnailGenerator.cpp")
    add_files("app/ui/thumbnail/ThumbnailListView.cpp")
    add_files("app/ui/thumbnail/ThumbnailContextMenu.cpp")

    -- Delegates
    add_files("app/delegate/PageNavigationDelegate.cpp")
    add_files("app/delegate/ThumbnailDelegate.cpp")



    -- Add all header files for proper organization
    add_headerfiles("app/*.h")
    add_headerfiles("app/ui/core/*.h")
    add_headerfiles("app/ui/viewer/*.h")
    add_headerfiles("app/ui/widgets/*.h")
    add_headerfiles("app/ui/dialogs/*.h")
    add_headerfiles("app/ui/thumbnail/*.h")
    add_headerfiles("app/ui/managers/*.h")
    add_headerfiles("app/managers/*.h")
    add_headerfiles("app/model/*.h")
    add_headerfiles("app/controller/*.h")
    add_headerfiles("app/delegate/*.h")
    add_headerfiles("app/cache/*.h")
    add_headerfiles("app/utils/*.h")
    add_headerfiles("app/plugin/*.h")
    add_headerfiles("app/factory/*.h")
    add_headerfiles("app/command/*.h")


    -- Custom rules for asset copying
    after_build(function (target)
        -- Copy assets/styles directory to build output
        local targetdir = target:targetdir()
        os.cp("assets/styles", path.join(targetdir, "styles"))
        print("Copied assets/styles to %s", path.join(targetdir, "styles"))
    end)

target_end()

-- Optional: Test target
if has_config("enable_tests") then
    target("sast-readium-tests")
        set_kind("binary")
        set_default(false)
        add_deps("sast-readium")
        add_files("tests/*.cpp")
        add_packages("pkgconfig::poppler-qt6", "spdlog")
    target_end()
end



-- Print build configuration
after_build(function (target)
    print("=== Build Configuration Summary ===")
    print("Build mode: %s", get_config("mode"))
    print("C++ Standard: cxx20")
    print("Platform: %s", get_config("plat"))
    print("Architecture: %s", get_config("arch"))

    local current_toolchain = get_config("toolchain")
    if current_toolchain == "auto" then
        current_toolchain = get_default_toolchain()
    end
    print("Toolchain: %s", current_toolchain)

    if is_msys2() then
        print("MSYS2 environment detected")
        print("MSYSTEM: %s", os.getenv("MSYSTEM"))
    end

    local qt_base = find_qt_installation()
    if qt_base then
        print("Qt installation: %s", qt_base)
    else
        print("Qt installation: Not found")
    end

    print("=====================================")
end)
