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
includes("xmake/modules/compiler.lua")
includes("xmake/modules/package.lua")
includes("xmake/modules/dependencies.lua")

-- Configure quiet mode (only if quiet option is available)
-- Note: set_loglevel may not be available in all contexts, so we skip this for now

-- Configure toolchain early
configure_toolchain()

-- Apply compiler flags globally
setup_compiler_flags()

-- Targets
includes("xmake/targets/libs.lua")
includes("xmake/targets/app.lua")

-- Tests
includes("xmake/tests/tests.lua")

-- Build summary and packaging
after_build(function (target)
    -- Show build summary for main target
    if target:name() == "sast-readium" or not has_config("quiet") then
        cprint("${bright}=== Build Configuration Summary ===${clear}")
        cprint("${green}Build mode:${clear} %s", get_config("mode"))
        cprint("${green}C++ Standard:${clear} cxx20")
        cprint("${green}Platform:${clear} %s", get_config("plat"))
        cprint("${green}Architecture:${clear} %s", get_config("arch"))

        local current_toolchain = get_config("toolchain")
        if current_toolchain == "auto" then
            current_toolchain = get_default_toolchain()
        end
        cprint("${green}Toolchain:${clear} %s", current_toolchain)

        if is_msys2() then
            cprint("${yellow}MSYS2 environment:${clear} %s", os.getenv("MSYSTEM"))
        end

        local qt_base = find_qt_installation()
        if qt_base then
            cprint("${green}Qt installation:${clear} %s", qt_base)
        else
            cprint("${red}Qt installation:${clear} Not found")
        end

        -- Show enabled features
        if has_config("enable_hardening") then
            cprint("${green}Hardening flags:${clear} ON")
        end
        if has_config("enable_lto") and is_mode("release") then
            cprint("${green}Link Time Optimization:${clear} ON")
        end
        if has_config("warnings_as_errors") then
            cprint("${yellow}Warnings as Errors:${clear} ON")
        end

        cprint("${bright}=====================================${clear}")
    end

    -- Create packages for main target
    if target:name() == "sast-readium" and has_config("enable_packaging") then
        create_packages(target)
    end
end)
