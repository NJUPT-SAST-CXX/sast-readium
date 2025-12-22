-- xmake/targets/app.lua
-- Main application target definition

includes("../modules/qt.lua")
includes("../modules/dependencies.lua")

-- Shared configuration for app targets
function app_config(target)
    set_languages("cxx20")

    -- Setup dependencies
    setup_qt_dependencies()
    setup_external_dependencies()

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

    -- Include directories
    add_includedirs("$(projectdir)", "$(projectdir)/app", "$(buildir)")
    add_includedirs("$(projectdir)/app/ui", "$(projectdir)/app/ui/core", "$(projectdir)/app/ui/viewer", "$(projectdir)/app/ui/widgets")
    add_includedirs("$(projectdir)/app/ui/dialogs", "$(projectdir)/app/ui/thumbnail", "$(projectdir)/app/ui/managers")
    add_includedirs("$(projectdir)/app/managers", "$(projectdir)/app/model", "$(projectdir)/app/controller", "$(projectdir)/app/delegate")
    add_includedirs("$(projectdir)/app/cache", "$(projectdir)/app/utils", "$(projectdir)/app/plugin")
    add_includedirs("$(projectdir)/app/factory", "$(projectdir)/app/command")

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
end

-- Static library containing all app logic and resources
target("app_lib")
    set_kind("static")
    add_rules("qt.moc", "qt.qrc", "qt.ts")

    -- Dependencies
    add_deps("ElaWidgetTools")

    app_config(target)

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

    -- Headers
    add_headerfiles("$(projectdir)/app/*.h")
    add_headerfiles("$(projectdir)/app/**/*.h")

    -- Sources (Grouped by component matching CMake discovery)
    -- Core UI
    add_files("$(projectdir)/app/MainWindow.cpp")
    add_files("$(projectdir)/app/ui/core/*.cpp")
    add_files("$(projectdir)/app/ui/viewer/*.cpp")
    add_files("$(projectdir)/app/ui/widgets/*.cpp")
    add_files("$(projectdir)/app/ui/dialogs/*.cpp")
    add_files("$(projectdir)/app/ui/thumbnail/*.cpp")
    add_files("$(projectdir)/app/ui/managers/*.cpp")

    -- Logic components
    add_files("$(projectdir)/app/managers/*.cpp")
    add_files("$(projectdir)/app/model/*.cpp")
    add_files("$(projectdir)/app/controller/*.cpp")
    add_files("$(projectdir)/app/delegate/*.cpp")
    add_files("$(projectdir)/app/cache/*.cpp")
    add_files("$(projectdir)/app/utils/*.cpp")
    add_files("$(projectdir)/app/plugin/*.cpp")
    add_files("$(projectdir)/app/factory/*.cpp")
    add_files("$(projectdir)/app/command/*.cpp")
    add_files("$(projectdir)/app/logging/*.cpp")
    add_files("$(projectdir)/app/search/*.cpp")
    add_files("$(projectdir)/app/adapters/*.cpp")

    -- Resources
    add_files("$(projectdir)/app/app.qrc")
    add_files("$(projectdir)/app/ela_ui.qrc")

    -- Translations
    add_files("$(projectdir)/app/i18n/*.ts")

target_end()

-- Main Executable
target("sast-readium")
    set_kind("binary")
    add_rules("qt.moc")

    add_deps("app_lib")

    app_config(target)

    add_files("$(projectdir)/app/main.cpp")
    set_targetdir("$(buildir)")

    -- Assets copy and post-build optimizations
    after_build(function (target)
        local targetdir = target:targetdir()
        os.cp(path.join(os.projectdir(), "assets/styles"), path.join(targetdir, "styles"))
        if not has_config("quiet") then
            cprint("${dim}Copied assets/styles to %s${clear}", path.join(targetdir, "styles"))
        end

        -- Apply UPX compression if enabled
        includes("../modules/package.lua")
        compress_with_upx(target)
    end)
target_end()

-- Legacy Executable
if has_config("legacy_ui") then
    target("app-legacy")
        set_kind("binary")
        add_rules("qt.moc")

        add_deps("app_lib")

        app_config(target)

        add_files("$(projectdir)/app/main.cpp")
        set_targetdir("$(buildir)")

        -- Assets copy
        after_build(function (target)
            local targetdir = target:targetdir()
            os.cp(path.join(os.projectdir(), "assets/styles"), path.join(targetdir, "styles"))
        end)
    target_end()
end
