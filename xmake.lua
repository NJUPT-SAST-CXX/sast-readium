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
add_requires("fmt")

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
    if not toolchain or toolchain == "auto" then
        toolchain = get_default_toolchain()
    end

    print("Selected toolchain: " .. toolchain)

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

-- Configure toolchain before defining targets
configure_toolchain()

-- Main application target
target("sast-readium")
    set_kind("binary")

    -- Add Qt rules for MOC processing
    add_rules("qt.widgetapp")

    if is_plat("macosx") then
        print("macOS: Using Frameworks path  (homebrew)")

        local poppler_base = "/opt/homebrew/opt/poppler-qt6"
        add_frameworks("QtCore", "QtGui", "QtWidgets", "QtSvg", "QtConcurrent")
        add_includedirs(poppler_base .. "/include")
        add_includedirs(poppler_base .. "/include/poppler")
        add_includedirs(poppler_base .. "/include/poppler/qt6")
        add_linkdirs(poppler_base .. "/lib")
        add_links("poppler-qt6")
    else
        add_packages("pkgconfig::poppler-qt6")
    end


    add_defines("QT_CORE_LIB", "QT_GUI_LIB", "QT_WIDGETS_LIB", "QT_SVG_LIB")
    if is_mode("release") then
        add_defines("QT_NO_DEBUG")
    end

    -- QGraphics PDF support
    if has_config("enable_qgraphics_pdf") then
        add_defines("ENABLE_QGRAPHICS_PDF_SUPPORT")
    end

    local moc_headers = {
        "app/cache/PDFCacheManager.h",
        "app/cache/UnifiedCacheSystem.h",
        "app/command/Commands.h",
        "app/controller/DocumentController.h",
        "app/controller/PageController.h",
        "app/delegate/PageNavigationDelegate.h",
        "app/delegate/ThumbnailDelegate.h",
        "app/factory/WidgetFactory.h",
        "app/managers/StyleManager.h",
        "app/managers/FileTypeIconManager.h",
        "app/managers/RecentFilesManager.h",
        "app/model/DocumentModel.h",
        "app/model/PageModel.h",
        "app/model/RenderModel.h",
        "app/model/PDFOutlineModel.h",
        "app/model/ThumbnailModel.h",
        "app/model/AsyncDocumentLoader.h",
        "app/model/SearchModel.h",
        "app/model/BookmarkModel.h",
        "app/model/AnnotationModel.h",
        "app/plugin/PluginManager.h",
        "app/ui/core/ViewWidget.h",
        "app/ui/core/StatusBar.h",
        "app/ui/core/SideBar.h",
        "app/ui/core/MenuBar.h",
        "app/ui/core/ToolBar.h",
        "app/ui/core/RightSideBar.h",
        "app/ui/dialogs/DocumentMetadataDialog.h",
        "app/ui/dialogs/DocumentComparison.h",
        "app/ui/managers/WelcomeScreenManager.h",
        "app/ui/thumbnail/ThumbnailListView.h",
        "app/ui/thumbnail/ThumbnailGenerator.h",
        "app/ui/thumbnail/ThumbnailWidget.h",
        "app/ui/thumbnail/ThumbnailContextMenu.h",
        "app/ui/viewer/PDFPrerenderer.h",
        "app/ui/viewer/PDFAnimations.h",
        "app/ui/viewer/PDFViewerEnhancements.h",
        "app/ui/viewer/QGraphicsPDFViewer.h",
        "app/ui/viewer/PDFViewer.h",
        "app/ui/viewer/PDFOutlineWidget.h",
        "app/ui/widgets/DocumentTabWidget.h",
        "app/ui/widgets/SearchWidget.h",
        "app/ui/widgets/BookmarkWidget.h",
        "app/ui/widgets/AnnotationToolbar.h",
        "app/ui/widgets/DebugLogPanel.h",
        "app/ui/widgets/RecentFileListWidget.h",
        "app/ui/widgets/WelcomeWidget.h",
        "app/utils/DocumentAnalyzer.h",
        "app/utils/Logger.h",
        "app/utils/LoggingConfig.h",
        "app/utils/LoggingMacros.h",
        "app/utils/LoggingManager.h",
        "app/utils/PDFUtilities.h",
        "app/utils/QtSpdlogBridge.h",
        "app/view/Views.h",
        "app/MainWindow.h"
    }
    add_files(moc_headers)


    -- Set output directory
    set_targetdir("$(builddir)")

    -- Add spdlog dependency
    add_packages("spdlog")
    add_packages("fmt")
    add_packages("qt")

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
    add_headerfiles(moc_headers)

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

        local qt_base = "/opt/homebrew/opt/qt"
        add_frameworks("QtCore", "QtGui", "QtWidgets", "QtSvg", "QtConcurrent")
        add_linkdirs(path.join(qt_base, "lib"))
        add_includedirs(path.join(qt_base, "include"))
    end

    -- Translation files (Qt rules disabled to avoid assertion errors)
    -- add_files("app/i18n/app_en.ts")
    -- add_files("app/i18n/app_zh.ts")
    -- add_rules("qt.ts")
    -- set_values("qt.ts.lupdate", "app/i18n/app_en.ts", "app/i18n/app_zh.ts")

    add_files("app/*.cpp")
    add_files("app/ui/**/*.cpp")
    add_files("app/model/*.cpp")
    add_files("app/controller/*.cpp")
    add_files("app/managers/*.cpp")
    add_files("app/cache/*.cpp")
    add_files("app/utils/*.cpp")
    add_files("app/plugin/*.cpp")
    add_files("app/factory/*.cpp")
    add_files("app/command/*.cpp")
    add_files("app/delegate/*.cpp")
    add_files("app/view/*.cpp")

    add_headerfiles("app/**/*.h")

    -- Custom rules for asset copying
    after_build(function (target)
        -- Copy assets/styles directory to build output
        local targetdir = target:targetdir()
        os.cp("assets/styles", path.join(targetdir, "styles"))
        print("Copied assets/styles to %s", path.join(targetdir, "styles"))
    end)

    local qt_base = get_config("qt_path")
    if not qt_base or qt_base == "" then
        -- :D
        qt_base = "D:/Program/Qt/6.8.3/mingw_64"
    end

    add_includedirs(path.join(qt_base, "include"))
    add_includedirs(path.join(qt_base, "include", "QtSvg"))
    add_linkdirs(path.join(qt_base, "lib"))
    add_links("Qt6Core", "Qt6Gui", "Qt6Widgets", "Qt6Svg")

target_end()

-- Optional: Test target
if has_config("enable_tests") then
    target("sast-readium-tests")
        set_kind("binary")
        set_default(false)
        add_deps("sast-readium")
        add_files("tests/*.cpp")

        add_packages("spdlog", "qt", "fmt")
        add_packages("pkgconfig::poppler-qt6")
        add_links("Qt6Core", "Qt6Gui", "Qt6Widgets")

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