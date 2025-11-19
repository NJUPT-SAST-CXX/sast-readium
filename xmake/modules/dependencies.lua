-- xmake/modules/dependencies.lua
-- Centralized dependency management for all targets

-- Function to setup external dependencies for a target
function setup_external_dependencies()
    -- Always use pkg-config for poppler-qt6 - it's more reliable
    add_packages("pkgconfig::poppler-qt6")
    add_packages("spdlog")

    -- Platform-specific system libraries
    if is_plat("windows") then
        -- Windows system libraries
        add_syslinks("user32", "gdi32", "shell32", "ole32", "oleaut32", "uuid", "advapi32")

        -- OpenGL libraries for Windows
        add_syslinks("opengl32", "glu32")

        -- Windows-specific defines
        add_defines("WIN32_LEAN_AND_MEAN", "NOMINMAX")

    elseif is_plat("linux") then
        -- Linux system libraries
        add_syslinks("pthread", "dl", "m")

        -- X11 and OpenGL libraries for Linux
        add_syslinks("X11", "GL", "GLU")

        -- Fontconfig for font management
        add_syslinks("fontconfig", "freetype")

    elseif is_plat("macosx") then
        -- macOS frameworks
        add_frameworks("CoreFoundation", "CoreServices", "CoreGraphics", "Foundation")
        add_frameworks("OpenGL", "Cocoa", "IOKit", "Carbon")

        -- Font and text frameworks
        add_frameworks("CoreText", "ImageIO")
    end
end

-- Function to setup development dependencies (for tests, examples, etc.)
function setup_development_dependencies()
    -- Test framework
    if has_package("gtest") then
        add_packages("gtest")
    end

    -- Benchmarking (if available)
    if has_package("benchmark") then
        add_packages("benchmark")
    end
end

-- Function to setup Qt dependencies with error handling
function setup_qt_dependencies()
    local qt_base = find_qt_installation()
    if not qt_base then
        print("Warning: Qt installation not found!")
        print("Please install Qt6 or specify qt_path option")
        return false
    end

    -- Call the main Qt setup function
    setup_qt_for_target()

    -- Additional Qt modules that might be needed
    local qt_modules = {
        "Qt6Network",
        "Qt6PrintSupport",
        "Qt6Xml"
    }

    -- Check and add optional Qt modules
    for _, module in ipairs(qt_modules) do
        local lib_path = ""
        if is_plat("windows") then
            lib_path = path.join(qt_base, "lib", module .. ".lib")
            if not os.isfile(lib_path) then
                lib_path = path.join(qt_base, "lib", "lib" .. module .. ".a")
            end
        elseif is_plat("linux") then
            lib_path = path.join(qt_base, "lib", "lib" .. module .. ".so")
            if not os.isfile(lib_path) then
                lib_path = path.join(qt_base, "lib", "lib" .. module .. ".a")
            end
        elseif is_plat("macosx") then
            lib_path = path.join(qt_base, "lib", module .. ".framework")
        end

        if os.isfile(lib_path) or os.isdir(lib_path) then
            add_links(module)
            if not get_config or not get_config("quiet") then
                print("Added optional Qt module: " .. module)
            end
        end
    end
end

-- Function to validate all dependencies before build
function validate_dependencies()
    import("lib.detect.find_program")
    local missing_deps = {}

    -- Check Qt installation
    local qt_base = find_qt_installation()
    if not qt_base then
        table.insert(missing_deps, "Qt6 (use qt_path option to specify location)")
    end

    -- Check pkg-config availability
    if not find_program("pkg-config") then
        table.insert(missing_deps, "pkg-config (required for poppler-qt6)")
    end

    -- Check poppler-qt6 via pkg-config
    local poppler_check = os.iorunv("pkg-config", {"--exists", "poppler-qt6"})
    if poppler_check ~= 0 then
        table.insert(missing_deps, "poppler-qt6 development packages")
    end

    -- Report missing dependencies
    if #missing_deps > 0 then
        print("Missing dependencies:")
        for _, dep in ipairs(missing_deps) do
            print("  ✗ " .. dep)
        end
        print("Please install missing dependencies before building")
        return false
    end

    if not get_config or not get_config("quiet") then
        print("✓ All dependencies validated")
    end
    return true
end
