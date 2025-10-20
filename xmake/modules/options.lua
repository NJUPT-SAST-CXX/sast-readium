-- xmake/modules/options.lua
-- Options and global packages

-- Build options
option("toolchain")
    set_default("auto")
    set_description("Select toolchain (auto, mingw, msvc, clang, gcc)")
    set_values("auto", "mingw", "msvc", "clang", "gcc")
    set_showmenu(true)
option_end()

option("qt_path")
    set_default("")
    set_description("Custom Qt installation path")
    set_showmenu(true)
option_end()

option("enable_clangd")
    set_default(true)
    set_description("Enable clangd configuration generation")
    set_showmenu(true)
option_end()

option("enable_qgraphics_pdf")
    set_default(false)
    set_description("Enable QGraphics-based PDF rendering support")
    set_showmenu(true)
option_end()

option("enable_tests")
    set_default(false)
    set_description("Enable building tests")
    set_showmenu(true)
option_end()

-- Global requirements
add_requires("pkgconfig::poppler-qt6")
add_requires("spdlog")
add_requires("gtest", {optional = true})
