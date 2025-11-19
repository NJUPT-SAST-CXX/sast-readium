-- xmake/modules/qt.lua
-- Enhanced Qt discovery and setup with automatic detection

-- Qt version information structure
local QtVersion = {
    path = "",
    version = "",
    major = 0,
    minor = 0,
    patch = 0,
    is_qt6 = false,
    is_qt5 = false,
    score = 0  -- Higher score = better preference
}

-- Create a new QtVersion instance
function QtVersion:new(path, version_string)
    local obj = {}
    setmetatable(obj, self)
    self.__index = self

    obj.path = path
    obj.version = version_string or "unknown"

    -- Parse version string (e.g., "6.5.0" or "5.15.2")
    if version_string then
        local major, minor, patch = version_string:match("(%d+)%.(%d+)%.(%d+)")
        if major then
            obj.major = tonumber(major)
            obj.minor = tonumber(minor)
            obj.patch = tonumber(patch)
            obj.is_qt6 = (obj.major == 6)
            obj.is_qt5 = (obj.major == 5)

            -- Calculate preference score
            obj.score = obj.major * 10000 + obj.minor * 100 + obj.patch
            -- Prefer Qt6 over Qt5
            if obj.is_qt6 then
                obj.score = obj.score + 100000
            end
        end
    end

    return obj
end

-- Check if a Qt installation is valid and get its version
function validate_qt_installation(qt_path)
    if not qt_path or not os.isdir(qt_path) then
        return nil
    end

    -- Check for Qt6 (more flexible detection)
    local is_qt6 = os.isdir(qt_path .. "/include/qt6") or
                   os.isfile(qt_path .. "/lib/cmake/Qt6/Qt6Config.cmake") or
                   os.isfile(qt_path .. "/lib/cmake/Qt6Core/Qt6CoreConfig.cmake")

    if is_qt6 then
        local version = detect_qt_version(qt_path, 6)
        return QtVersion:new(qt_path, version)
    end

    -- Check for Qt5 (more flexible detection)
    local is_qt5 = os.isdir(qt_path .. "/include/QtCore") or
                   os.isfile(qt_path .. "/lib/cmake/Qt5/Qt5Config.cmake") or
                   os.isfile(qt_path .. "/lib/cmake/Qt5Core/Qt5CoreConfig.cmake")

    if is_qt5 then
        local version = detect_qt_version(qt_path, 5)
        return QtVersion:new(qt_path, version)
    end

    return nil
end

-- Detect Qt version from installation
function detect_qt_version(qt_path, major_version)
    -- Try to read version from qmake
    local qmake_paths = {
        qt_path .. "/bin/qmake",
        qt_path .. "/bin/qmake" .. major_version,
        qt_path .. "/libexec/qmake"
    }

    for _, qmake_path in ipairs(qmake_paths) do
        if os.isfile(qmake_path) then
            local result = os.iorunv(qmake_path, {"-query", "QT_VERSION"})
            if result == 0 then
                local version_output = os.iorun(qmake_path .. " -query QT_VERSION")
                if version_output then
                    local version = version_output:match("([%d%.]+)")
                    if version then
                        return version
                    end
                end
            end
        end
    end

    -- Try to read version from CMake config files
    local cmake_config_paths = {
        qt_path .. "/lib/cmake/Qt" .. major_version .. "Core/Qt" .. major_version .. "CoreConfigVersion.cmake",
        qt_path .. "/lib/cmake/Qt" .. major_version .. "/Qt" .. major_version .. "ConfigVersion.cmake"
    }

    for _, config_path in ipairs(cmake_config_paths) do
        if os.isfile(config_path) then
            local content = io.readfile(config_path)
            if content then
                local version = content:match('set%s*%(%s*PACKAGE_VERSION%s+"([%d%.]+)"')
                if version then
                    return version
                end
            end
        end
    end

    -- Fallback to major version
    return major_version .. ".0.0"
end

-- Get Qt paths from environment variables
function get_qt_paths_from_env()
    local paths = {}

    -- Standard Qt environment variables
    local env_vars = {
        "QTDIR",
        "QT_ROOT",
        "Qt6_DIR",
        "Qt5_DIR",
        "QT6_ROOT_PATH",
        "QT5_ROOT_PATH"
    }

    for _, var in ipairs(env_vars) do
        local value = os.getenv(var)
        if value and value ~= "" then
            -- Qt6_DIR and Qt5_DIR usually point to lib/cmake/Qt6, we need the root
            if var:match("_DIR$") then
                -- Extract root path from cmake dir (e.g., /path/to/qt/lib/cmake/Qt6 -> /path/to/qt)
                local root = value:match("(.+)/lib/cmake/Qt[56]/?$")
                if root then
                    table.insert(paths, root)
                else
                    table.insert(paths, value)
                end
            else
                table.insert(paths, value)
            end
        end
    end

    -- Parse CMAKE_PREFIX_PATH for Qt installations
    local cmake_prefix = os.getenv("CMAKE_PREFIX_PATH")
    if cmake_prefix then
        -- Split by path separator (: on Unix, ; on Windows)
        local separator = is_plat("windows") and ";" or ":"
        for path in cmake_prefix:gmatch("[^" .. separator .. "]+") do
            if path and path ~= "" then
                table.insert(paths, path)
            end
        end
    end

    -- Check PATH for Qt bin directories
    local path_env = os.getenv("PATH")
    if path_env then
        local separator = is_plat("windows") and ";" or ":"
        for path in path_env:gmatch("[^" .. separator .. "]+") do
            if path and path:match("[Qq]t") and os.isdir(path) then
                -- If this is a Qt bin directory, get the parent
                local qt_root = path:match("(.+)/bin/?$") or path:match("(.+)\\bin\\?$")
                if qt_root and os.isdir(qt_root) then
                    table.insert(paths, qt_root)
                end
            end
        end
    end

    return paths
end

-- Get platform-specific Qt search paths
function get_platform_qt_paths()
    local paths = {}

    if is_plat("windows") then
        -- Windows-specific Qt search paths
        local windows_paths = {
            -- Qt Company official installations
            "C:/Qt",
            "D:/Qt",
            -- Program Files locations
            "C:/Program Files/Qt",
            "C:/Program Files (x86)/Qt",
            -- MSYS2/MinGW locations
            "C:/msys64/mingw64",
            "C:/msys64/mingw32",
            "D:/msys64/mingw64",
            "D:/msys64/mingw32",
            -- vcpkg locations
            "C:/vcpkg/installed/x64-windows",
            "C:/vcpkg/installed/x86-windows",
            "C:/tools/vcpkg/installed/x64-windows",
            -- Chocolatey locations
            "C:/ProgramData/chocolatey/lib/qt-opensource/tools",
            -- User profile locations
            os.getenv("USERPROFILE") .. "/Qt",
            os.getenv("LOCALAPPDATA") .. "/Qt"
        }

        -- Add version-specific paths for common Qt versions
        local qt_versions = {"6.7.0", "6.6.0", "6.5.0", "6.4.0", "5.15.2", "5.15.1", "5.15.0"}
        local qt_compilers = {"msvc2022_64", "msvc2019_64", "mingw_64", "mingw81_64"}

        for _, base_path in ipairs({"C:/Qt", "D:/Qt"}) do
            for _, version in ipairs(qt_versions) do
                for _, compiler in ipairs(qt_compilers) do
                    table.insert(paths, base_path .. "/" .. version .. "/" .. compiler)
                end
            end
        end

        -- Add base paths
        for _, path in ipairs(windows_paths) do
            if path then
                table.insert(paths, path)
            end
        end

    elseif is_plat("linux") then
        -- Linux-specific Qt search paths
        local linux_paths = {
            -- System package manager locations
            "/usr",
            "/usr/local",
            -- Distribution-specific locations
            "/opt/qt6",
            "/opt/qt5",
            "/opt/Qt",
            "/opt/Qt6",
            "/opt/Qt5",
            -- Snap package locations
            "/snap/qt5-core20/current",
            "/snap/qt6-core22/current",
            -- Flatpak locations (if accessible)
            "/var/lib/flatpak/runtime/org.kde.Platform",
            -- AppImage locations
            "/opt/appimage/qt",
            -- User installations
            os.getenv("HOME") .. "/Qt",
            os.getenv("HOME") .. "/.local/qt",
            os.getenv("HOME") .. "/opt/qt"
        }

        -- Add version-specific paths
        local qt_versions = {"6.7.0", "6.6.0", "6.5.0", "6.4.0", "5.15.2"}
        local qt_compilers = {"gcc_64", "gcc_32"}

        for _, version in ipairs(qt_versions) do
            for _, compiler in ipairs(qt_compilers) do
                table.insert(paths, os.getenv("HOME") .. "/Qt/" .. version .. "/" .. compiler)
                table.insert(paths, "/opt/Qt/" .. version .. "/" .. compiler)
            end
        end

        for _, path in ipairs(linux_paths) do
            if path then
                table.insert(paths, path)
            end
        end

    elseif is_plat("macosx") then
        -- macOS-specific Qt search paths
        local macos_paths = {
            -- Homebrew locations
            "/opt/homebrew/opt/qt@6",
            "/opt/homebrew/opt/qt@5",
            "/opt/homebrew/opt/qt",
            "/usr/local/opt/qt@6",
            "/usr/local/opt/qt@5",
            "/usr/local/opt/qt",
            -- MacPorts locations
            "/opt/local/libexec/qt6",
            "/opt/local/libexec/qt5",
            "/opt/local",
            -- System locations
            "/usr/local",
            -- Qt Company installations
            "/Applications/Qt",
            -- User installations
            os.getenv("HOME") .. "/Qt",
            os.getenv("HOME") .. "/Applications/Qt"
        }

        -- Add version-specific paths for macOS
        local qt_versions = {"6.7.0", "6.6.0", "6.5.0", "6.4.0", "5.15.2"}

        for _, version in ipairs(qt_versions) do
            table.insert(paths, os.getenv("HOME") .. "/Qt/" .. version .. "/macos")
            table.insert(paths, "/Applications/Qt/" .. version .. "/macos")
        end

        for _, path in ipairs(macos_paths) do
            if path then
                table.insert(paths, path)
            end
        end
    end

    return paths
end

-- Enhanced Qt installation finder with comprehensive detection
function find_qt_installation()
    local verbose = not get_config or not get_config("quiet")
    local found_installations = {}

    -- 1. Check custom qt_path configuration first (highest priority)
    local custom_qt = get_config("qt_path")
    if custom_qt and custom_qt ~= "" then
        if verbose then
            print("Checking custom Qt path: " .. custom_qt)
        end
        local qt_info = validate_qt_installation(custom_qt)
        if qt_info then
            if verbose then
                print("Found Qt " .. qt_info.version .. " at custom path: " .. custom_qt)
            end
            return custom_qt
        else
            print("Warning: Custom Qt path is invalid: " .. custom_qt)
        end
    end

    -- 2. Check environment variables
    if verbose then
        print("Searching Qt installations from environment variables...")
    end
    local env_paths = get_qt_paths_from_env()
    for _, path in ipairs(env_paths) do
        local qt_info = validate_qt_installation(path)
        if qt_info then
            table.insert(found_installations, qt_info)
            if verbose then
                print("Found Qt " .. qt_info.version .. " from environment: " .. path)
            end
        end
    end

    -- 3. Check platform-specific paths
    if verbose then
        print("Searching Qt installations in platform-specific locations...")
    end
    local platform_paths = get_platform_qt_paths()
    for _, path in ipairs(platform_paths) do
        local qt_info = validate_qt_installation(path)
        if qt_info then
            table.insert(found_installations, qt_info)
            if verbose then
                print("Found Qt " .. qt_info.version .. " at: " .. path)
            end
        end
    end

    -- 4. If no installations found, return nil
    if #found_installations == 0 then
        if verbose then
            print("No Qt installations found")
        end
        return nil
    end

    -- 5. Sort installations by preference score (highest first)
    table.sort(found_installations, function(a, b)
        return a.score > b.score
    end)

    -- 6. Return the best installation
    local best_qt = found_installations[1]
    if verbose then
        print("Selected Qt " .. best_qt.version .. " at: " .. best_qt.path)
        if #found_installations > 1 then
            print("Other Qt installations found:")
            for i = 2, math.min(#found_installations, 5) do
                local qt = found_installations[i]
                print("  Qt " .. qt.version .. " at: " .. qt.path)
            end
        end
    end

    return best_qt.path
end

-- Get detailed Qt installation information
function get_qt_installation_info()
    local qt_path = find_qt_installation()
    if not qt_path then
        return nil
    end

    return validate_qt_installation(qt_path)
end

-- Provide helpful error messages and solutions when Qt is not found
function provide_qt_installation_help()
    print("\n=== Qt Installation Not Found ===")
    print("The build system could not locate a Qt installation.")
    print("\nPossible solutions:")

    if is_plat("windows") then
        print("1. Install Qt from https://www.qt.io/download")
        print("2. Install Qt via vcpkg: vcpkg install qt6")
        print("3. Install Qt via MSYS2: pacman -S mingw-w64-x86_64-qt6")
        print("4. Set custom path: xmake config --qt_path=C:/path/to/qt")
        print("5. Set environment variable: set QTDIR=C:/path/to/qt")
    elseif is_plat("linux") then
        print("1. Install via package manager:")
        print("   Ubuntu/Debian: sudo apt install qt6-base-dev")
        print("   Fedora: sudo dnf install qt6-qtbase-devel")
        print("   Arch: sudo pacman -S qt6-base")
        print("2. Install from https://www.qt.io/download")
        print("3. Set custom path: xmake config --qt_path=/path/to/qt")
        print("4. Set environment variable: export QTDIR=/path/to/qt")
    elseif is_plat("macosx") then
        print("1. Install via Homebrew: brew install qt@6")
        print("2. Install from https://www.qt.io/download")
        print("3. Set custom path: xmake config --qt_path=/path/to/qt")
        print("4. Set environment variable: export QTDIR=/path/to/qt")
    end

    print("\nFor more help, visit: https://doc.qt.io/qt-6/gettingstarted.html")
    print("=====================================\n")
end

-- Validate that Qt installation has required components
function validate_qt_components(qt_path, required_modules)
    if not qt_path or not os.isdir(qt_path) then
        return false, "Qt path does not exist"
    end

    required_modules = required_modules or {"Core", "Gui", "Widgets"}
    local missing_modules = {}
    local qt_info = validate_qt_installation(qt_path)

    if not qt_info then
        return false, "Invalid Qt installation"
    end

    -- Check for required Qt modules
    for _, module in ipairs(required_modules) do
        local module_found = false

        -- Check include directories
        local include_paths = {
            qt_path .. "/include/Qt" .. module,
            qt_path .. "/include/qt6/Qt" .. module,
            qt_path .. "/include/qt5/Qt" .. module
        }

        for _, include_path in ipairs(include_paths) do
            if os.isdir(include_path) then
                module_found = true
                break
            end
        end

        -- Check library files
        if not module_found then
            local lib_patterns = {
                qt_path .. "/lib/libQt" .. (qt_info.is_qt6 and "6" or "5") .. module .. ".*",
                qt_path .. "/lib/Qt" .. (qt_info.is_qt6 and "6" or "5") .. module .. ".*"
            }

            for _, pattern in ipairs(lib_patterns) do
                local files = os.files(pattern)
                if files and #files > 0 then
                    module_found = true
                    break
                end
            end
        end

        if not module_found then
            table.insert(missing_modules, module)
        end
    end

    if #missing_modules > 0 then
        return false, "Missing Qt modules: " .. table.concat(missing_modules, ", ")
    end

    return true, "Qt installation is valid"
end

-- Enhanced Qt installation finder with fallback mechanisms
function find_qt_installation_with_fallbacks()
    local qt_path = find_qt_installation()

    if qt_path then
        -- Validate the found installation
        local valid, error_msg = validate_qt_components(qt_path)
        if valid then
            return qt_path
        else
            print("Warning: Found Qt installation has issues: " .. error_msg)
            print("Continuing search for alternative installations...")
        end
    end

    -- If no valid installation found, provide help
    provide_qt_installation_help()
    return nil
end

function setup_qt_for_target()
    local verbose = not get_config or not get_config("quiet")

    -- Use enhanced Qt detection with fallbacks
    local qt_base = find_qt_installation_with_fallbacks()
    if not qt_base then
        if verbose then
            print("Error: No valid Qt installation found.")
        end
        return false
    end

    -- Get detailed Qt information
    local qt_info = validate_qt_installation(qt_base)
    if not qt_info then
        if verbose then
            print("Error: Qt installation validation failed.")
        end
        return false
    end

    if verbose then
        print("Using Qt " .. qt_info.version .. " (" .. (qt_info.is_qt6 and "Qt6" or "Qt5") .. ") from: " .. qt_base)
    end

    -- Configure Qt based on detected version and platform
    local qt_version_prefix = qt_info.is_qt6 and "Qt6" or "Qt5"
    local qt_include_base = qt_base .. "/include"

    -- Determine include directory structure
    local qt_include_dir = qt_include_base
    if os.isdir(qt_include_base .. "/qt6") then
        qt_include_dir = qt_include_base .. "/qt6"
    elseif os.isdir(qt_include_base .. "/qt5") then
        qt_include_dir = qt_include_base .. "/qt5"
    end

    if is_plat("windows") then
        -- Add Qt include directories
        add_includedirs(qt_include_dir)
        add_includedirs(qt_include_dir .. "/QtCore")
        add_includedirs(qt_include_dir .. "/QtGui")
        add_includedirs(qt_include_dir .. "/QtWidgets")
        add_includedirs(qt_include_dir .. "/QtSvg")
        add_includedirs(qt_include_dir .. "/QtConcurrent")

        -- Optional modules
        if os.isdir(qt_include_dir .. "/QtOpenGL") then
            add_includedirs(qt_include_dir .. "/QtOpenGL")
        end
        if os.isdir(qt_include_dir .. "/QtOpenGLWidgets") then
            add_includedirs(qt_include_dir .. "/QtOpenGLWidgets")
        end
        if os.isdir(qt_include_dir .. "/QtNetwork") then
            add_includedirs(qt_include_dir .. "/QtNetwork")
        end
        if os.isdir(qt_include_dir .. "/QtPrintSupport") then
            add_includedirs(qt_include_dir .. "/QtPrintSupport")
        end

        -- Add library directory and core libraries
        add_linkdirs(qt_base .. "/lib")
        add_links(qt_version_prefix .. "Core", qt_version_prefix .. "Gui",
                 qt_version_prefix .. "Widgets", qt_version_prefix .. "Svg",
                 qt_version_prefix .. "Concurrent")

        -- Add optional libraries if available
        local optional_libs = {"OpenGL", "OpenGLWidgets", "Network", "PrintSupport"}
        for _, lib in ipairs(optional_libs) do
            local lib_patterns = {
                qt_base .. "/lib/lib" .. qt_version_prefix .. lib .. ".a",
                qt_base .. "/lib/" .. qt_version_prefix .. lib .. ".dll.a",
                qt_base .. "/lib/" .. qt_version_prefix .. lib .. ".lib"
            }

            for _, pattern in ipairs(lib_patterns) do
                if os.isfile(pattern) then
                    add_links(qt_version_prefix .. lib)
                    if lib == "OpenGL" then
                        add_syslinks("opengl32")
                    end
                    if verbose then
                        print("Added optional Qt library: " .. qt_version_prefix .. lib)
                    end
                    break
                end
            end
        end
    elseif is_plat("linux") then
        -- Add Qt include directories
        add_includedirs(qt_include_dir)
        add_includedirs(qt_include_dir .. "/QtCore")
        add_includedirs(qt_include_dir .. "/QtGui")
        add_includedirs(qt_include_dir .. "/QtWidgets")
        add_includedirs(qt_include_dir .. "/QtSvg")
        add_includedirs(qt_include_dir .. "/QtConcurrent")

        -- Optional modules
        local optional_modules = {"QtOpenGL", "QtOpenGLWidgets", "QtNetwork", "QtPrintSupport"}
        for _, module in ipairs(optional_modules) do
            if os.isdir(qt_include_dir .. "/" .. module) then
                add_includedirs(qt_include_dir .. "/" .. module)
            end
        end

        -- Add library directory and core libraries
        add_linkdirs(qt_base .. "/lib")
        add_links(qt_version_prefix .. "Core", qt_version_prefix .. "Gui",
                 qt_version_prefix .. "Widgets", qt_version_prefix .. "Svg",
                 qt_version_prefix .. "Concurrent")

        -- Add optional libraries if available
        local optional_libs = {"OpenGL", "OpenGLWidgets", "Network", "PrintSupport"}
        for _, lib in ipairs(optional_libs) do
            local lib_patterns = {
                qt_base .. "/lib/lib" .. qt_version_prefix .. lib .. ".so",
                qt_base .. "/lib/lib" .. qt_version_prefix .. lib .. ".a"
            }

            for _, pattern in ipairs(lib_patterns) do
                if os.isfile(pattern) then
                    add_links(qt_version_prefix .. lib)
                    if verbose then
                        print("Added optional Qt library: " .. qt_version_prefix .. lib)
                    end
                    break
                end
            end
        end
    elseif is_plat("macosx") then
        -- Add Qt include directories
        add_includedirs(qt_include_dir)
        add_includedirs(qt_include_dir .. "/QtCore")
        add_includedirs(qt_include_dir .. "/QtGui")
        add_includedirs(qt_include_dir .. "/QtWidgets")
        add_includedirs(qt_include_dir .. "/QtSvg")
        add_includedirs(qt_include_dir .. "/QtConcurrent")

        -- Optional modules
        local optional_modules = {"QtOpenGL", "QtOpenGLWidgets", "QtNetwork", "QtPrintSupport"}
        for _, module in ipairs(optional_modules) do
            if os.isdir(qt_include_dir .. "/" .. module) then
                add_includedirs(qt_include_dir .. "/" .. module)
            end
        end

        -- Add library directory and core libraries
        add_linkdirs(qt_base .. "/lib")
        add_links(qt_version_prefix .. "Core", qt_version_prefix .. "Gui",
                 qt_version_prefix .. "Widgets", qt_version_prefix .. "Svg",
                 qt_version_prefix .. "Concurrent")

        -- Add optional libraries (macOS typically has them available)
        local optional_libs = {"OpenGL", "OpenGLWidgets", "Network", "PrintSupport"}
        for _, lib in ipairs(optional_libs) do
            local framework_path = qt_base .. "/lib/" .. qt_version_prefix .. lib .. ".framework"
            local lib_path = qt_base .. "/lib/lib" .. qt_version_prefix .. lib .. ".dylib"

            if os.isdir(framework_path) or os.isfile(lib_path) then
                add_links(qt_version_prefix .. lib)
                if verbose then
                    print("Added optional Qt library: " .. qt_version_prefix .. lib)
                end
            end
        end
    end

    -- Add Qt defines
    add_defines("QT_CORE_LIB", "QT_GUI_LIB", "QT_WIDGETS_LIB", "QT_SVG_LIB")

    -- Define OpenGL macros if available
    if os.isdir(qt_include_dir .. "/QtOpenGL") then
        add_defines("QT_OPENGL_LIB")
    end
    if os.isdir(qt_include_dir .. "/QtOpenGLWidgets") then
        add_defines("QT_OPENGLWIDGETS_LIB")
    end

    -- Define Network and PrintSupport macros if available
    if os.isdir(qt_include_dir .. "/QtNetwork") then
        add_defines("QT_NETWORK_LIB")
    end
    if os.isdir(qt_include_dir .. "/QtPrintSupport") then
        add_defines("QT_PRINTSUPPORT_LIB")
    end

    -- Build mode specific defines
    if is_mode("release") then
        add_defines("QT_NO_DEBUG")
    end

    -- Feature-specific defines
    if get_config and has_config("enable_qgraphics_pdf") then
        add_defines("ENABLE_QGRAPHICS_PDF_SUPPORT")
    end

    -- Enhanced MinGW compatibility (Windows only)
    if is_plat("windows") then
        local tc = get_config("toolchain")
        if tc == "auto" then tc = get_default_toolchain() end
        if tc == "mingw" then
            local mingw_root = qt_base
            if path.filename(qt_base) == "qt6-static" then
                mingw_root = path.directory(qt_base)
            end

            -- Add MinGW include root (contains stdlib.h in MSYS2)
            if os.isdir(mingw_root .. "/include") then
                -- Use -idirafter to avoid shadowing compiler built-in paths
                add_cxflags("-idirafter", mingw_root .. "/include", {force = true})
                add_cxxflags("-idirafter", mingw_root .. "/include", {force = true})
            end

            -- Add GCC internal include directories
            local gcc_includes = os.dirs(mingw_root .. "/lib/gcc/*/*/include")
            if gcc_includes and #gcc_includes > 0 then
                add_cxflags("-idirafter", gcc_includes[1], {force = true})
                add_cxxflags("-idirafter", gcc_includes[1], {force = true})
            end

            -- Add target-specific headers for MSYS2
            local triplet_include = mingw_root .. "/x86_64-w64-mingw32/include"
            if os.isdir(triplet_include) then
                add_cxflags("-idirafter", triplet_include, {force = true})
                add_cxxflags("-idirafter", triplet_include, {force = true})
            end
        end
    end

    if verbose then
        print("Qt setup completed successfully")
    end

    return true
end
