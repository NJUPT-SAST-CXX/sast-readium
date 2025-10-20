-- xmake.lua (modular)

-- Minimum version
set_xmakever("2.8.0")

-- Project metadata
set_project("sast-readium")
set_version("0.1.0.0")
set_description("SAST Readium - A Qt6-based PDF reader application")

-- Global language/diagnostics
set_languages("cxx20")
set_warnings("all")

-- Modes
add_rules("mode.debug", "mode.release")

-- Include modules
includes("xmake/modules/options.lua")
includes("xmake/modules/toolchains.lua")
includes("xmake/modules/qt.lua")

-- Configure toolchain early
configure_toolchain()

-- Targets
includes("xmake/targets/app.lua")

-- Tests
includes("xmake/tests/tests.lua")

-- Build summary
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
