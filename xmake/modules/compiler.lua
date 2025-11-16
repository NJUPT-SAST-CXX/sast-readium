-- xmake/modules/compiler.lua
-- Compiler flags and hardening configuration

function setup_compiler_flags()
    -- Enable colored diagnostics
    if is_mode("debug", "release") then
        add_cxxflags("-fdiagnostics-color=always", {tools = {"gcc", "clang"}})
        add_cxxflags("/diagnostics:caret", {tools = "cl"})
    end

    -- Base warning flags
    if is_plat("windows") then
        if get_config("toolchain") == "msvc" or (get_config("toolchain") == "auto" and not is_msys2()) then
            add_cxxflags("/W4", "/permissive-", "/sdl", {force = true})
            if has_config("warnings_as_errors") then
                add_cxxflags("/WX", {force = true})
            end
        else
            add_cxxflags("-Wall", "-Wextra", "-Wpedantic", {force = true})
            if has_config("warnings_as_errors") then
                add_cxxflags("-Werror", {force = true})
            end
        end
    else
        add_cxxflags("-Wall", "-Wextra", "-Wpedantic", {force = true})
        if has_config("warnings_as_errors") then
            add_cxxflags("-Werror", {force = true})
        end
    end

    -- Security hardening flags
    if has_config("enable_hardening") then
        if is_plat("windows") then
            if get_config("toolchain") == "msvc" or (get_config("toolchain") == "auto" and not is_msys2()) then
                -- MSVC hardening
                add_ldflags("/guard:cf", {force = true})
                add_cxxflags("/GS", {force = true})
            else
                -- MinGW hardening
                add_cxxflags("-D_FORTIFY_SOURCE=2", "-fstack-protector-strong", {force = true})
            end
        elseif is_plat("linux") then
            -- Linux hardening
            add_cxxflags("-D_FORTIFY_SOURCE=2", "-fstack-protector-strong", "-fno-plt", {force = true})
            add_ldflags("-Wl,-z,relro", "-Wl,-z,now", {force = true})
        elseif is_plat("macosx") then
            -- macOS hardening
            add_cxxflags("-D_FORTIFY_SOURCE=2", "-fstack-protector-strong", {force = true})
        end
    end

    -- Link Time Optimization (release only)
    if has_config("enable_lto") and is_mode("release") then
        set_policy("build.optimization.lto", true)
    end

    -- Position Independent Code
    add_cxxflags("-fPIC", {tools = {"gcc", "clang"}})
end
