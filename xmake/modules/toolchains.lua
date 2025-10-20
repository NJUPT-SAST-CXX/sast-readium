-- xmake/modules/toolchains.lua
-- Toolchain helpers and configuration

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
