-- xmake/targets/libs.lua
-- Library targets for ElaWidgetTools and ElaPacketIO

includes("../modules/qt.lua")

-- ElaWidgetTools library target
target("ElaWidgetTools")
    if has_config("enable_static_libs") then
        set_kind("static")
    else
        set_kind("shared")
    end
    set_languages("cxx20")

    -- Add Qt MOC rule for Qt components
    add_rules("qt.moc")

    -- Setup Qt for this target
    setup_qt_for_target()

    -- Source files
    add_files("$(projectdir)/libs/ElaWidgetTools/*.cpp")
    add_files("$(projectdir)/libs/ElaWidgetTools/private/*.cpp")
    add_files("$(projectdir)/libs/ElaWidgetTools/DeveloperComponents/*.cpp")

    -- Headers requiring MOC (Qt objects)
    add_files("$(projectdir)/libs/ElaWidgetTools/ElaApplication.h")
    add_files("$(projectdir)/libs/ElaWidgetTools/ElaWidget.h")
    add_files("$(projectdir)/libs/ElaWidgetTools/ElaWindow.h")
    add_files("$(projectdir)/libs/ElaWidgetTools/ElaEventBus.h")
    add_files("$(projectdir)/libs/ElaWidgetTools/ElaNavigationRouter.h")
    add_files("$(projectdir)/libs/ElaWidgetTools/ElaKeyBinder.h")
    add_files("$(projectdir)/libs/ElaWidgetTools/ElaTheme.h")
    add_files("$(projectdir)/libs/ElaWidgetTools/ElaGraphicsScene.h")
    add_files("$(projectdir)/libs/ElaWidgetTools/ElaGraphicsView.h")
    add_files("$(projectdir)/libs/ElaWidgetTools/ElaGraphicsItem.h")
    add_files("$(projectdir)/libs/ElaWidgetTools/ElaGraphicsLineItem.h")

    -- All widget headers that likely inherit from QWidget/QObject
    add_files("$(projectdir)/libs/ElaWidgetTools/Ela*.h", {rule = "qt.moc"})
    add_files("$(projectdir)/libs/ElaWidgetTools/DeveloperComponents/Ela*.h", {rule = "qt.moc"})

    -- Resource files
    add_files("$(projectdir)/libs/ElaWidgetTools/ElaWidgetTools.qrc")

    -- Include directories
    add_includedirs("$(projectdir)/libs/ElaWidgetTools", {public = true})
    add_includedirs("$(projectdir)/libs/ElaWidgetTools/private", {public = true})
    add_includedirs("$(projectdir)/libs/ElaWidgetTools/DeveloperComponents", {public = true})

    -- Defines
    add_defines("ELAWIDGETTOOLS_LIBRARY", {public = true})

    -- Platform-specific settings
    if is_plat("windows") then
        add_defines("WIN32", "_WINDOWS", "UNICODE", "_UNICODE")
        local current_toolchain = get_config("toolchain")
        if current_toolchain == "auto" then current_toolchain = get_default_toolchain() end
        if current_toolchain == "mingw" then
            set_targetdir("$(buildir)/libs")
        elseif current_toolchain == "msvc" then
            add_ldflags("/SUBSYSTEM:WINDOWS")
            set_targetdir("$(buildir)/libs")
        end
        add_syslinks("D3D11", "DXGI")
    elseif is_plat("linux") then
        add_defines("LINUX")
        add_syslinks("pthread", "dl")
        set_targetdir("$(buildir)/libs")
    elseif is_plat("macosx") then
        add_defines("MACOS")
        add_frameworks("CoreFoundation", "CoreServices")
        set_targetdir("$(buildir)/libs")
    end

    -- Build mode settings
    if is_mode("release") then
        set_optimize("fastest")
        add_defines("NDEBUG")
        set_symbols("hidden")

        -- Size optimization: dead code elimination
        add_cxxflags("-ffunction-sections", "-fdata-sections", {tools = {"gcc", "clang"}})
        if is_plat("windows", "linux") then
            add_ldflags("-Wl,--gc-sections", {tools = {"gcc", "clang"}})
        elseif is_plat("macosx") then
            add_ldflags("-Wl,-dead_strip", {tools = {"gcc", "clang"}})
        end

        -- Strip shared libraries if requested
        if has_config("strip_binaries") then
            add_ldflags("-s", {tools = {"gcc", "clang"}})
        end
    else
        set_optimize("none")
        -- Use split debug info in debug mode for smaller binaries
        if has_config("split_debug_info") then
            add_cxxflags("-g1", {tools = {"gcc", "clang"}})
        else
            set_symbols("debug")
        end
        add_defines("DEBUG", "_DEBUG")
    end

    -- Installation rules
    on_install(function (target)
        local installdir = target:installdir()
        os.cp(target:targetfile(), path.join(installdir, "lib"))
        os.cp("$(projectdir)/libs/ElaWidgetTools/*.h", path.join(installdir, "include/ElaWidgetTools"))
        os.cp("$(projectdir)/libs/ElaWidgetTools/DeveloperComponents/*.h", path.join(installdir, "include/ElaWidgetTools/DeveloperComponents"))
    end)
target_end()

-- ElaPacketIO library target
target("ElaPacketIO")
    if has_config("enable_static_libs") then
        set_kind("static")
    else
        set_kind("shared")
    end
    set_languages("cxx20")

    -- Source files
    add_files("$(projectdir)/libs/ElaPacketIO/*.cpp")
    add_files("$(projectdir)/libs/ElaPacketIO/PacketIO/*.cpp")
    add_files("$(projectdir)/libs/ElaPacketIO/GenIO/*.cpp")
    add_files("$(projectdir)/libs/ElaPacketIO/Util/*.cpp")
    add_files("$(projectdir)/libs/ElaPacketIO/XIO/*.cpp")

    -- Include directories
    add_includedirs("$(projectdir)/libs/ElaPacketIO", {public = true})
    add_includedirs("$(projectdir)/libs/ElaPacketIO/PacketIO", {public = true})
    add_includedirs("$(projectdir)/libs/ElaPacketIO/GenIO", {public = true})
    add_includedirs("$(projectdir)/libs/ElaPacketIO/Util", {public = true})
    add_includedirs("$(projectdir)/libs/ElaPacketIO/XIO", {public = true})

    -- Defines
    add_defines("ELAPACKETIO_LIBRARY", {public = true})

    -- Platform-specific settings
    if is_plat("windows") then
        add_defines("WIN32", "_WINDOWS", "UNICODE", "_UNICODE")
        local current_toolchain = get_config("toolchain")
        if current_toolchain == "auto" then current_toolchain = get_default_toolchain() end
        if current_toolchain == "mingw" then
            set_targetdir("$(buildir)/libs")
        elseif current_toolchain == "msvc" then
            set_targetdir("$(buildir)/libs")
        end
    elseif is_plat("linux") then
        add_defines("LINUX")
        add_syslinks("pthread", "dl")
        set_targetdir("$(buildir)/libs")
    elseif is_plat("macosx") then
        add_defines("MACOS")
        add_frameworks("CoreFoundation", "CoreServices")
        set_targetdir("$(buildir)/libs")
    end

    -- Build mode settings
    if is_mode("release") then
        set_optimize("fastest")
        add_defines("NDEBUG")
        set_symbols("hidden")

        -- Size optimization: dead code elimination
        add_cxxflags("-ffunction-sections", "-fdata-sections", {tools = {"gcc", "clang"}})
        if is_plat("windows", "linux") then
            add_ldflags("-Wl,--gc-sections", {tools = {"gcc", "clang"}})
        elseif is_plat("macosx") then
            add_ldflags("-Wl,-dead_strip", {tools = {"gcc", "clang"}})
        end

        -- Strip shared libraries if requested
        if has_config("strip_binaries") then
            add_ldflags("-s", {tools = {"gcc", "clang"}})
        end
    else
        set_optimize("none")
        -- Use split debug info in debug mode for smaller binaries
        if has_config("split_debug_info") then
            add_cxxflags("-g1", {tools = {"gcc", "clang"}})
        else
            set_symbols("debug")
        end
        add_defines("DEBUG", "_DEBUG")
    end

    -- Installation rules
    on_install(function (target)
        local installdir = target:installdir()
        os.cp(target:targetfile(), path.join(installdir, "lib"))
        os.cp("$(projectdir)/libs/ElaPacketIO/*.h", path.join(installdir, "include/ElaPacketIO"))
        os.cp("$(projectdir)/libs/ElaPacketIO/PacketIO/*.h", path.join(installdir, "include/ElaPacketIO/PacketIO"))
        os.cp("$(projectdir)/libs/ElaPacketIO/GenIO/*.h", path.join(installdir, "include/ElaPacketIO/GenIO"))
        os.cp("$(projectdir)/libs/ElaPacketIO/Util/*.h", path.join(installdir, "include/ElaPacketIO/Util"))
        os.cp("$(projectdir)/libs/ElaPacketIO/XIO/*.h", path.join(installdir, "include/ElaPacketIO/XIO"))
    end)
target_end()

-- ElaWidgetToolsExample executable target (optional)
if has_config("enable_examples") then
    target("ElaWidgetToolsExample")
        set_kind("binary")
        set_languages("cxx20")
        set_default(false)

        -- Add Qt MOC rule
        add_rules("qt.moc")

        -- Dependencies
        add_deps("ElaWidgetTools")

        -- Setup Qt
        setup_qt_for_target()

        -- Source files
        add_files("$(projectdir)/libs/ElaWidgetToolsExample/*.cpp")
        add_files("$(projectdir)/libs/ElaWidgetToolsExample/ModelView/*.cpp")
        add_files("$(projectdir)/libs/ElaWidgetToolsExample/ExamplePage/*.cpp")

        -- Headers requiring MOC
        add_files("$(projectdir)/libs/ElaWidgetToolsExample/*.h", {rule = "qt.moc"})
        add_files("$(projectdir)/libs/ElaWidgetToolsExample/ModelView/*.h", {rule = "qt.moc"})
        add_files("$(projectdir)/libs/ElaWidgetToolsExample/ExamplePage/*.h", {rule = "qt.moc"})

        -- Resource files
        add_files("$(projectdir)/libs/ElaWidgetToolsExample/ElaWidgetToolsExample.qrc")

        -- Include directories
        add_includedirs("$(projectdir)/libs/ElaWidgetToolsExample")
        add_includedirs("$(projectdir)/libs/ElaWidgetToolsExample/ModelView")
        add_includedirs("$(projectdir)/libs/ElaWidgetToolsExample/ExamplePage")

        -- Platform-specific settings
        if is_plat("windows") then
            add_defines("WIN32", "_WINDOWS", "UNICODE", "_UNICODE")
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

        -- Build mode settings
        if is_mode("release") then
            set_optimize("fastest")
            add_defines("NDEBUG")
            set_symbols("hidden")
        else
            set_optimize("none")
            set_symbols("debug")
            add_defines("DEBUG", "_DEBUG")
        end

        set_targetdir("$(buildir)/examples")
    target_end()
end
