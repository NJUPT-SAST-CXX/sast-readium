-- xmake/targets/app.lua
-- Main application target definition

includes("../modules/qt.lua")

target("sast-readium")
    set_kind("binary")
    add_rules("qt.moc")

    -- Headers requiring MOC
    add_files("$(projectdir)/app/MainWindow.h")
    add_files("$(projectdir)/app/model/DocumentModel.h")
    add_files("$(projectdir)/app/model/PageModel.h")
    add_files("$(projectdir)/app/model/RenderModel.h")
    add_files("$(projectdir)/app/model/PDFOutlineModel.h")
    add_files("$(projectdir)/app/controller/DocumentController.h")
    add_files("$(projectdir)/app/controller/PageController.h")
    add_files("$(projectdir)/app/managers/StyleManager.h")
    add_files("$(projectdir)/app/managers/FileTypeIconManager.h")
    add_files("$(projectdir)/app/managers/RecentFilesManager.h")
    add_files("$(projectdir)/app/ui/viewer/PDFViewer.h")
    add_files("$(projectdir)/app/ui/viewer/PDFOutlineWidget.h")
    add_files("$(projectdir)/app/ui/widgets/DocumentTabWidget.h")
    add_files("$(projectdir)/app/ui/widgets/SearchWidget.h")
    add_files("$(projectdir)/app/ui/widgets/BookmarkWidget.h")
    add_files("$(projectdir)/app/ui/widgets/AnnotationToolbar.h")
    add_files("$(projectdir)/app/model/ThumbnailModel.h")
    add_files("$(projectdir)/app/delegate/ThumbnailDelegate.h")
    add_files("$(projectdir)/app/ui/thumbnail/ThumbnailListView.h")
    add_files("$(projectdir)/app/ui/thumbnail/ThumbnailGenerator.h")
    add_files("$(projectdir)/app/model/AsyncDocumentLoader.h")
    add_files("$(projectdir)/app/ui/dialogs/DocumentMetadataDialog.h")
    add_files("$(projectdir)/app/ui/dialogs/DocumentComparison.h")
    add_files("$(projectdir)/app/ui/core/ViewWidget.h")
    add_files("$(projectdir)/app/ui/core/StatusBar.h")
    add_files("$(projectdir)/app/ui/core/SideBar.h")
    add_files("$(projectdir)/app/ui/core/MenuBar.h")
    add_files("$(projectdir)/app/ui/core/ToolBar.h")
    add_files("$(projectdir)/app/ui/core/ContextMenuManager.h")
    add_files("$(projectdir)/app/factory/WidgetFactory.h")
    add_files("$(projectdir)/app/model/SearchModel.h")
    add_files("$(projectdir)/app/model/BookmarkModel.h")
    add_files("$(projectdir)/app/model/AnnotationModel.h")
    add_files("$(projectdir)/app/ui/viewer/PDFPrerenderer.h")
    add_files("$(projectdir)/app/ui/viewer/PDFAnimations.h")

    add_files("$(projectdir)/app/ui/viewer/QGraphicsPDFViewer.h")
    add_files("$(projectdir)/app/cache/PDFCacheManager.h")
    add_files("$(projectdir)/app/plugin/PluginManager.h")
    add_files("$(projectdir)/app/delegate/PageNavigationDelegate.h")
    add_files("$(projectdir)/app/utils/DocumentAnalyzer.h")
    add_files("$(projectdir)/app/ui/thumbnail/ThumbnailWidget.h")
    add_files("$(projectdir)/app/ui/thumbnail/ThumbnailContextMenu.h")

    set_targetdir("$(builddir)")

    -- Setup Qt
    setup_qt_for_target()

    -- External packages
    -- On Windows+MinGW, pkg-config injects the generic Mingw include root
    -- (e.g. .../mingw64/include) which breaks libstdc++'s `#include_next <stdlib.h>`.
    -- To avoid that, we manually add only the specific sub-include directories
    -- for Poppler and friends on MinGW, and still use pkg-config elsewhere.
    do
        local current_toolchain = get_config("toolchain")
        if current_toolchain == "auto" then current_toolchain = get_default_toolchain() end
        if is_plat("windows") and current_toolchain == "mingw" then
            -- Derive MSYS2 MinGW root from Qt base
            local qt_base = find_qt_installation()
            local mingw_root = qt_base
            if qt_base and path.filename(qt_base) == "qt6-static" then
                mingw_root = path.directory(qt_base)
            end
            if not mingw_root or not os.isdir(mingw_root .. "/include") then
                -- Fallback to environment if detection failed
                mingw_root = os.getenv("MINGW_PREFIX") or "D:/msys64/mingw64"
            end
            -- Add only the required third-party include subfolders
            add_includedirs(mingw_root .. "/include/poppler/qt6", {public = true})
            add_includedirs(mingw_root .. "/include/poppler", {public = true})
            add_includedirs(mingw_root .. "/include/freetype2", {public = true})
            add_includedirs(mingw_root .. "/include/harfbuzz", {public = true})
            add_includedirs(mingw_root .. "/include/glib-2.0", {public = true})
            add_includedirs(mingw_root .. "/lib/glib-2.0/include", {public = true})
            add_includedirs(mingw_root .. "/include/nss3", {public = true})
            add_includedirs(mingw_root .. "/include/nspr", {public = true})
            add_includedirs(mingw_root .. "/include/openjpeg-2.5", {public = true})
            add_includedirs(mingw_root .. "/include/libpng16", {public = true})
            add_includedirs(mingw_root .. "/include/webp", {public = true})
            -- Link directories; we still rely on the linker to resolve poppler and its deps
            add_linkdirs(mingw_root .. "/lib")
            -- Base poppler libs (the rest are pulled transitively)
            add_links("poppler-qt6", "poppler")
        else
            add_packages("pkgconfig::poppler-qt6")
        end
        -- spdlog is safe everywhere
        add_packages("spdlog")
    end

    -- Generate config.h
    before_build(function (target)
        local config_content = [[#pragma once

#define PROJECT_NAME "sast-readium"
#define APP_NAME "SAST Readium - A Qt6-based PDF reader application"
#define PROJECT_VER "0.1.0.0"
#define PROJECT_VER_MAJOR "0"
#define PROJECT_VER_MINOR "1"
#define PROJECT_VER_PATCH "0"
]]
        io.writefile(path.join(os.projectdir(), "app/config.h"), config_content)
        if not has_config("quiet") then
            cprint("${dim}Generated config.h${clear}")
        end
    end)

    -- Include directories
    add_includedirs("$(projectdir)", "$(projectdir)/app", "$(builddir)")
    add_includedirs("$(projectdir)/app/ui", "$(projectdir)/app/ui/core", "$(projectdir)/app/ui/viewer", "$(projectdir)/app/ui/widgets")
    add_includedirs("$(projectdir)/app/ui/dialogs", "$(projectdir)/app/ui/thumbnail", "$(projectdir)/app/ui/managers")
    add_includedirs("$(projectdir)/app/managers", "$(projectdir)/app/model", "$(projectdir)/app/controller", "$(projectdir)/app/delegate")
    add_includedirs("$(projectdir)/app/cache", "$(projectdir)/app/utils", "$(projectdir)/app/plugin")
    add_includedirs("$(projectdir)/app/factory", "$(projectdir)/app/command")

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

    -- Platform-specific
    if is_plat("windows") then
        add_defines("WIN32", "_WINDOWS", "UNICODE", "_UNICODE")
    add_files("$(projectdir)/app/app.rc")
        local current_toolchain = get_config("toolchain")
        if current_toolchain == "auto" then current_toolchain = get_default_toolchain() end
        if current_toolchain == "msvc" then
            add_ldflags("/SUBSYSTEM:WINDOWS")
        end
    elseif is_plat("linux") then
        add_defines("LINUX")
        add_syslinks("pthread", "dl")
    elseif is_plat("macosx") then
        add_defines("MACOS")
        add_frameworks("CoreFoundation", "CoreServices")
    end

    -- Sources
    add_files("$(projectdir)/app/main.cpp")
    add_files("$(projectdir)/app/MainWindow.cpp")

    add_files("$(projectdir)/app/ui/core/ViewWidget.cpp")
    add_files("$(projectdir)/app/ui/core/StatusBar.cpp")
    add_files("$(projectdir)/app/ui/core/SideBar.cpp")
    add_files("$(projectdir)/app/ui/core/MenuBar.cpp")
    add_files("$(projectdir)/app/ui/core/ToolBar.cpp")
    add_files("$(projectdir)/app/ui/core/ContextMenuManager.cpp")

    add_files("$(projectdir)/app/model/DocumentModel.cpp")
    add_files("$(projectdir)/app/model/PageModel.cpp")
    add_files("$(projectdir)/app/model/RenderModel.cpp")
    add_files("$(projectdir)/app/model/PDFOutlineModel.cpp")
    add_files("$(projectdir)/app/model/AsyncDocumentLoader.cpp")
    add_files("$(projectdir)/app/model/SearchModel.cpp")
    add_files("$(projectdir)/app/model/BookmarkModel.cpp")
    add_files("$(projectdir)/app/model/AnnotationModel.cpp")
    add_files("$(projectdir)/app/model/ThumbnailModel.cpp")

    add_files("$(projectdir)/app/controller/DocumentController.cpp")
    add_files("$(projectdir)/app/controller/PageController.cpp")

    add_files("$(projectdir)/app/managers/StyleManager.cpp")
    add_files("$(projectdir)/app/managers/FileTypeIconManager.cpp")
    add_files("$(projectdir)/app/managers/RecentFilesManager.cpp")

    add_files("$(projectdir)/app/cache/PDFCacheManager.cpp")

    add_files("$(projectdir)/app/utils/PDFUtilities.cpp")
    add_files("$(projectdir)/app/utils/DocumentAnalyzer.cpp")

    add_files("$(projectdir)/app/plugin/PluginManager.cpp")

    add_files("$(projectdir)/app/factory/WidgetFactory.cpp")


    add_files("$(projectdir)/app/ui/viewer/PDFViewer.cpp")
    add_files("$(projectdir)/app/ui/viewer/PDFOutlineWidget.cpp")

    add_files("$(projectdir)/app/ui/viewer/PDFAnimations.cpp")
    add_files("$(projectdir)/app/ui/viewer/PDFPrerenderer.cpp")
    add_files("$(projectdir)/app/ui/viewer/QGraphicsPDFViewer.cpp")

    add_files("$(projectdir)/app/ui/widgets/DocumentTabWidget.cpp")
    add_files("$(projectdir)/app/ui/widgets/SearchWidget.cpp")
    add_files("$(projectdir)/app/ui/widgets/BookmarkWidget.cpp")
    add_files("$(projectdir)/app/ui/widgets/AnnotationToolbar.cpp")

    add_files("$(projectdir)/app/ui/dialogs/DocumentComparison.cpp")
    add_files("$(projectdir)/app/ui/dialogs/DocumentMetadataDialog.cpp")

    add_files("$(projectdir)/app/ui/thumbnail/ThumbnailWidget.cpp")
    add_files("$(projectdir)/app/ui/thumbnail/ThumbnailGenerator.cpp")
    add_files("$(projectdir)/app/ui/thumbnail/ThumbnailListView.cpp")
    add_files("$(projectdir)/app/ui/thumbnail/ThumbnailContextMenu.cpp")

    add_files("$(projectdir)/app/delegate/PageNavigationDelegate.cpp")
    add_files("$(projectdir)/app/delegate/ThumbnailDelegate.cpp")

    -- Headers (organization only)
    add_headerfiles("$(projectdir)/app/*.h")
    add_headerfiles("$(projectdir)/app/ui/core/*.h")
    add_headerfiles("$(projectdir)/app/ui/viewer/*.h")
    add_headerfiles("$(projectdir)/app/ui/widgets/*.h")
    add_headerfiles("$(projectdir)/app/ui/dialogs/*.h")
    add_headerfiles("$(projectdir)/app/ui/thumbnail/*.h")
    add_headerfiles("$(projectdir)/app/ui/managers/*.h")
    add_headerfiles("$(projectdir)/app/managers/*.h")
    add_headerfiles("$(projectdir)/app/model/*.h")
    add_headerfiles("$(projectdir)/app/controller/*.h")
    add_headerfiles("$(projectdir)/app/delegate/*.h")
    add_headerfiles("$(projectdir)/app/cache/*.h")
    add_headerfiles("$(projectdir)/app/utils/*.h")
    add_headerfiles("$(projectdir)/app/plugin/*.h")
    add_headerfiles("$(projectdir)/app/factory/*.h")
    add_headerfiles("$(projectdir)/app/command/*.h")

    -- Assets copy
    after_build(function (target)
        local targetdir = target:targetdir()
        os.cp(path.join(os.projectdir(), "assets/styles"), path.join(targetdir, "styles"))
        if not has_config("quiet") then
            cprint("${dim}Copied assets/styles to %s${clear}", path.join(targetdir, "styles"))
        end
    end)

target_end()
