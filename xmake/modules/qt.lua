-- xmake/modules/qt.lua
-- Qt discovery and setup

function find_qt_installation()
    local custom_qt = get_config("qt_path")
    if custom_qt and custom_qt ~= "" then
        return custom_qt
    end

    if is_plat("windows") then
        local qt_paths = {
            "D:/msys64/mingw64",
            "C:/Qt/6.5.0/mingw_64",
            "C:/Qt/6.5.0/msvc2022_64",
            "C:/Qt/Tools/mingw1120_64",
        }
        for _, qt_path in ipairs(qt_paths) do
            if os.isdir(qt_path .. "/include/qt6") then
                return qt_path
            end
        end
    elseif is_plat("linux") then
        local qt_paths = {
            "/usr",
            "/usr/local",
            "/opt/qt6",
            os.getenv("HOME") .. "/Qt/6.5.0/gcc_64"
        }
        for _, qt_path in ipairs(qt_paths) do
            if os.isdir(qt_path .. "/include/qt6") or os.isdir(qt_path .. "/include/QtCore") then
                return qt_path
            end
        end
    elseif is_plat("macosx") then
        local qt_paths = {
            "/usr/local",
            "/opt/homebrew",
            "/opt/qt6",
            os.getenv("HOME") .. "/Qt/6.5.0/macos"
        }
        for _, qt_path in ipairs(qt_paths) do
            if os.isdir(qt_path .. "/include/qt6") or os.isdir(qt_path .. "/include/QtCore") then
                return qt_path
            end
        end
    end
    return nil
end

function setup_qt_for_target()
    local qt_base = find_qt_installation()
    if not qt_base then
        print("Warning: Qt installation not found. Please specify qt_path option.")
        return
    end

    if is_plat("windows") then
        add_includedirs(qt_base .. "/include/qt6")
        add_includedirs(qt_base .. "/include/qt6/QtCore")
        add_includedirs(qt_base .. "/include/qt6/QtGui")
        add_includedirs(qt_base .. "/include/qt6/QtWidgets")
        add_includedirs(qt_base .. "/include/qt6/QtSvg")
        add_includedirs(qt_base .. "/include/qt6/QtConcurrent")
        -- OpenGL modules (Qt6)
        if os.isdir(qt_base .. "/include/qt6/QtOpenGL") then
            add_includedirs(qt_base .. "/include/qt6/QtOpenGL")
        end
        if os.isdir(qt_base .. "/include/qt6/QtOpenGLWidgets") then
            add_includedirs(qt_base .. "/include/qt6/QtOpenGLWidgets")
        end
        add_linkdirs(qt_base .. "/lib")
        add_links("Qt6Core", "Qt6Gui", "Qt6Widgets", "Qt6Svg", "Qt6Concurrent")
        if os.isfile(qt_base .. "/lib/libQt6OpenGL.a") or os.isfile(qt_base .. "/lib/Qt6OpenGL.dll.a") or os.isfile(qt_base .. "/lib/Qt6OpenGL.lib") then
            add_links("Qt6OpenGL")
            add_syslinks("opengl32")
        end
        if os.isfile(qt_base .. "/lib/libQt6OpenGLWidgets.a") or os.isfile(qt_base .. "/lib/Qt6OpenGLWidgets.dll.a") or os.isfile(qt_base .. "/lib/Qt6OpenGLWidgets.lib") then
            add_links("Qt6OpenGLWidgets")
        end
    elseif is_plat("linux") then
        if os.isdir(qt_base .. "/include/qt6") then
            add_includedirs(qt_base .. "/include/qt6")
            add_includedirs(qt_base .. "/include/qt6/QtCore")
            add_includedirs(qt_base .. "/include/qt6/QtGui")
            add_includedirs(qt_base .. "/include/qt6/QtWidgets")
            add_includedirs(qt_base .. "/include/qt6/QtSvg")
            add_includedirs(qt_base .. "/include/qt6/QtConcurrent")
            if os.isdir(qt_base .. "/include/qt6/QtOpenGL") then
                add_includedirs(qt_base .. "/include/qt6/QtOpenGL")
            end
            if os.isdir(qt_base .. "/include/qt6/QtOpenGLWidgets") then
                add_includedirs(qt_base .. "/include/qt6/QtOpenGLWidgets")
            end
        else
            add_includedirs(qt_base .. "/include/QtCore")
            add_includedirs(qt_base .. "/include/QtGui")
            add_includedirs(qt_base .. "/include/QtWidgets")
            add_includedirs(qt_base .. "/include/QtSvg")
            add_includedirs(qt_base .. "/include/QtConcurrent")
            if os.isdir(qt_base .. "/include/QtOpenGL") then
                add_includedirs(qt_base .. "/include/QtOpenGL")
            end
            if os.isdir(qt_base .. "/include/QtOpenGLWidgets") then
                add_includedirs(qt_base .. "/include/QtOpenGLWidgets")
            end
        end
        add_linkdirs(qt_base .. "/lib")
        add_links("Qt6Core", "Qt6Gui", "Qt6Widgets", "Qt6Svg", "Qt6Concurrent")
        if os.isfile(qt_base .. "/lib/libQt6OpenGL.so") or os.isfile(qt_base .. "/lib/libQt6OpenGL.a") then
            add_links("Qt6OpenGL")
        end
        if os.isfile(qt_base .. "/lib/libQt6OpenGLWidgets.so") or os.isfile(qt_base .. "/lib/libQt6OpenGLWidgets.a") then
            add_links("Qt6OpenGLWidgets")
        end
    elseif is_plat("macosx") then
        if os.isdir(qt_base .. "/include/qt6") then
            add_includedirs(qt_base .. "/include/qt6")
            add_includedirs(qt_base .. "/include/qt6/QtCore")
            add_includedirs(qt_base .. "/include/qt6/QtGui")
            add_includedirs(qt_base .. "/include/qt6/QtWidgets")
            add_includedirs(qt_base .. "/include/qt6/QtSvg")
            add_includedirs(qt_base .. "/include/qt6/QtConcurrent")
            if os.isdir(qt_base .. "/include/qt6/QtOpenGL") then
                add_includedirs(qt_base .. "/include/qt6/QtOpenGL")
            end
            if os.isdir(qt_base .. "/include/qt6/QtOpenGLWidgets") then
                add_includedirs(qt_base .. "/include/qt6/QtOpenGLWidgets")
            end
        else
            add_includedirs(qt_base .. "/include/QtCore")
            add_includedirs(qt_base .. "/include/QtGui")
            add_includedirs(qt_base .. "/include/QtWidgets")
            add_includedirs(qt_base .. "/include/QtSvg")
            add_includedirs(qt_base .. "/include/QtConcurrent")
            if os.isdir(qt_base .. "/include/QtOpenGL") then
                add_includedirs(qt_base .. "/include/QtOpenGL")
            end
            if os.isdir(qt_base .. "/include/QtOpenGLWidgets") then
                add_includedirs(qt_base .. "/include/QtOpenGLWidgets")
            end
        end
        add_linkdirs(qt_base .. "/lib")
        add_links("Qt6Core", "Qt6Gui", "Qt6Widgets", "Qt6Svg", "Qt6Concurrent")
        add_links("Qt6OpenGL", "Qt6OpenGLWidgets")
    end

    add_defines("QT_CORE_LIB", "QT_GUI_LIB", "QT_WIDGETS_LIB", "QT_SVG_LIB")
    -- Define OpenGL macros if available
    if os.isdir(qt_base .. "/include/qt6/QtOpenGL") or os.isdir(qt_base .. "/include/QtOpenGL") then
        add_defines("QT_OPENGL_LIB")
    end
    if os.isdir(qt_base .. "/include/qt6/QtOpenGLWidgets") or os.isdir(qt_base .. "/include/QtOpenGLWidgets") then
        add_defines("QT_OPENGLWIDGETS_LIB")
    end
    if is_mode("release") then
        add_defines("QT_NO_DEBUG")
    end

    if has_config("enable_qgraphics_pdf") then
        add_defines("ENABLE_QGRAPHICS_PDF_SUPPORT")
    end

    -- Ensure MinGW standard headers are discoverable (fix cstdlib -> stdlib.h include_next)
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
                -- Do NOT add as -isystem to avoid shadowing the compiler's built-in
                -- system include path ordering which breaks #include_next.
                -- Instead, append it after the standard search paths.
                add_cxflags("-idirafter", mingw_root .. "/include", {force = true})
                add_cxxflags("-idirafter", mingw_root .. "/include", {force = true})
            end
            -- Add GCC internal include (contains headers referenced by include_next)
            local gcc_includes = os.dirs(mingw_root .. "/lib/gcc/*/*/include")
            if gcc_includes and #gcc_includes > 0 then
                -- Append it after the libstdc++ include dirs
                add_cxflags("-idirafter", gcc_includes[1], {force = true})
                add_cxxflags("-idirafter", gcc_includes[1], {force = true})
            end
            -- Some MSYS2 layouts provide target-specific headers here
            local triplet_include = mingw_root .. "/x86_64-w64-mingw32/include"
            if os.isdir(triplet_include) then
                add_cxflags("-idirafter", triplet_include, {force = true})
                add_cxxflags("-idirafter", triplet_include, {force = true})
            end
        end
    end
end
