-- xmake/modules/package.lua
-- Cross-platform packaging support for xmake

-- Package options
option("enable_packaging")
    set_default(true)
    set_description("Enable packaging support")
    set_showmenu(true)
option_end()

option("package_format")
    set_default("auto")
    set_description("Package format (auto, nsis, zip, deb, rpm, dmg, appimage)")
    set_values("auto", "nsis", "zip", "deb", "rpm", "dmg", "appimage", "targz")
    set_showmenu(true)
option_end()

-- Detect appropriate package formats for current platform
function get_default_package_formats()
    local formats = {}

    if is_plat("windows") then
        -- Windows: NSIS installer + ZIP portable
        table.insert(formats, "nsis")
        table.insert(formats, "zip")
    elseif is_plat("linux") then
        -- Linux: DEB, RPM, TGZ, AppImage
        table.insert(formats, "deb")
        table.insert(formats, "rpm")
        table.insert(formats, "targz")
        table.insert(formats, "appimage")
    elseif is_plat("macosx") then
        -- macOS: DMG, TGZ
        table.insert(formats, "dmg")
        table.insert(formats, "targz")
    end

    return formats
end

-- Generate package metadata
function generate_package_metadata()
    local metadata = {
        name = "sast-readium",
        version = "0.1.0.0",
        description = "SAST Readium - Qt6-based PDF reader",
        homepage = "https://github.com/NJUPT-SAST/sast-readium",
        license = "MIT",
        author = "SAST Team"
    }
    return metadata
end

-- Create NSIS installer (Windows)
function create_nsis_package(target)
    if not is_plat("windows") then
        return
    end

    cprint("${bright cyan}Creating NSIS installer...${clear}")

    local target_file = target:targetfile()
    local target_dir = path.directory(target_file)
    local output_dir = path.join(os.projectdir(), "package")

    -- Ensure output directory exists
    os.mkdir(output_dir)

    -- Use existing NSIS script if available
    local nsis_script = path.join(os.projectdir(), "distrib", "pack.nsi")
    if os.isfile(nsis_script) then
        local nsis_tool = find_program("makensis")
        if nsis_tool then
            os.execv(nsis_tool, {nsis_script}, {curdir = path.join(os.projectdir(), "distrib")})
            cprint("${green}NSIS installer created${clear}")
        else
            cprint("${yellow}makensis not found - skipping NSIS package${clear}")
        end
    end
end

-- Create ZIP portable package
function create_zip_package(target)
    cprint("${bright cyan}Creating ZIP portable package...${clear}")

    local metadata = generate_package_metadata()
    local target_file = target:targetfile()
    local target_dir = path.directory(target_file)
    local output_dir = path.join(os.projectdir(), "package")
    local arch = os.arch()

    os.mkdir(output_dir)

    -- Create temporary packaging directory
    local temp_dir = path.join(output_dir, "temp_portable")
    local app_dir = path.join(temp_dir, string.format("SAST-Readium-%s", metadata.version))

    os.rm(temp_dir)
    os.mkdir(app_dir)
    os.mkdir(path.join(app_dir, "bin"))
    os.mkdir(path.join(app_dir, "styles"))
    os.mkdir(path.join(app_dir, "docs"))

    -- Copy executable
    os.cp(target_file, path.join(app_dir, "bin"))

    -- Copy DLLs on Windows
    if is_plat("windows") then
        local dll_files = os.files(path.join(target_dir, "*.dll"))
        for _, dll in ipairs(dll_files) do
            os.cp(dll, path.join(app_dir, "bin"))
        end

        -- Copy Qt plugins
        local plugin_dirs = {"platforms", "imageformats", "iconengines", "styles"}
        for _, plugin_dir in ipairs(plugin_dirs) do
            local src = path.join(target_dir, plugin_dir)
            if os.isdir(src) then
                os.cp(src, path.join(app_dir, "bin", plugin_dir))
            end
        end
    end

    -- Copy resources
    local styles_dir = path.join(os.projectdir(), "assets", "styles")
    if os.isdir(styles_dir) then
        os.cp(path.join(styles_dir, "*"), path.join(app_dir, "styles"))
    end

    -- Copy documentation
    os.cp(path.join(os.projectdir(), "README.md"), path.join(app_dir, "docs"))
    os.cp(path.join(os.projectdir(), "LICENSE"), path.join(app_dir, "docs"))

    -- Create portable marker
    io.writefile(path.join(app_dir, "PORTABLE_MODE.txt"),
                 "This is a portable installation of SAST Readium.\\nNo installation required.")

    -- Create ZIP
    local zip_name = string.format("SAST-Readium-%s-%s-%s-portable.zip",
                                    metadata.version,
                                    is_plat("windows") and "Windows" or (is_plat("linux") and "Linux" or "macOS"),
                                    arch)
    local zip_path = path.join(output_dir, zip_name)

    os.cd(temp_dir)
    if is_plat("windows") then
        os.execv("powershell", {
            "-Command",
            string.format("Compress-Archive -Path '%s' -DestinationPath '%s' -Force",
                         path.filename(app_dir), zip_path)
        })
    else
        os.execv("zip", {"-r", zip_path, path.filename(app_dir)})
    end
    os.cd(os.projectdir())

    -- Cleanup
    os.rm(temp_dir)

    cprint("${green}Portable ZIP created: %s${clear}", zip_name)

    -- Generate checksum
    generate_checksum(zip_path)
end

-- Create TGZ package
function create_targz_package(target)
    cprint("${bright cyan}Creating TAR.GZ package...${clear}")

    local metadata = generate_package_metadata()
    local target_file = target:targetfile()
    local output_dir = path.join(os.projectdir(), "package")
    local arch = os.arch()

    os.mkdir(output_dir)

    local targz_name = string.format("SAST-Readium-%s-%s-%s.tar.gz",
                                      metadata.version,
                                      is_plat("linux") and "Linux" or "macOS",
                                      arch)
    local targz_path = path.join(output_dir, targz_name)

    -- Create temporary directory
    local temp_dir = path.join(output_dir, "temp_targz")
    local app_dir = path.join(temp_dir, string.format("sast-readium-%s", metadata.version))

    os.rm(temp_dir)
    os.mkdir(path.join(app_dir, "bin"))
    os.mkdir(path.join(app_dir, "share", "applications"))
    os.mkdir(path.join(app_dir, "share", "icons"))

    -- Copy executable
    os.cp(target_file, path.join(app_dir, "bin"))

    -- Copy resources
    local styles_dir = path.join(os.projectdir(), "assets", "styles")
    if os.isdir(styles_dir) then
        os.cp(styles_dir, path.join(app_dir, "share"))
    end

    -- Create desktop file for Linux
    if is_plat("linux") then
        io.writefile(path.join(app_dir, "share", "applications", "sast-readium.desktop"), [[
[Desktop Entry]
Type=Application
Name=SAST Readium
Comment=Qt6-based PDF reader
Exec=sast-readium
Icon=sast-readium
Categories=Office;Viewer;
Terminal=false
]])
    end

    -- Create tarball
    os.cd(temp_dir)
    os.execv("tar", {"-czf", targz_path, path.filename(app_dir)})
    os.cd(os.projectdir())

    -- Cleanup
    os.rm(temp_dir)

    cprint("${green}TAR.GZ created: %s${clear}", targz_name)
    generate_checksum(targz_path)
end

-- Create DMG package (macOS)
function create_dmg_package(target)
    if not is_plat("macosx") then
        return
    end

    cprint("${bright cyan}Creating DMG package...${clear}")

    local metadata = generate_package_metadata()
    local output_dir = path.join(os.projectdir(), "package")

    os.mkdir(output_dir)

    local dmg_name = string.format("SAST-Readium-%s-macOS.dmg", metadata.version)
    local dmg_path = path.join(output_dir, dmg_name)

    -- Create DMG using hdiutil
    local app_bundle = target:targetfile() .. ".app"
    if os.isdir(app_bundle) then
        os.execv("hdiutil", {"create", "-volname", "SAST Readium",
                            "-srcfolder", app_bundle,
                            "-ov", "-format", "UDZO",
                            dmg_path})
        cprint("${green}DMG created: %s${clear}", dmg_name)
        generate_checksum(dmg_path)
    else
        cprint("${yellow}App bundle not found - skipping DMG creation${clear}")
    end
end

-- Create DEB package (Debian/Ubuntu)
function create_deb_package(target)
    if not is_plat("linux") then
        return
    end

    cprint("${bright cyan}Creating DEB package...${clear}")

    local metadata = generate_package_metadata()
    local target_file = target:targetfile()
    local output_dir = path.join(os.projectdir(), "package")
    local arch = os.arch()
    if arch == "x86_64" then arch = "amd64" end -- deb uses amd64

    os.mkdir(output_dir)

    -- Create temporary packaging directory
    local temp_dir = path.join(output_dir, "temp_deb")
    local package_root = path.join(temp_dir, string.format("%s_%s_%s", metadata.name, metadata.version, arch))

    os.rm(temp_dir)
    os.mkdir(package_root)

    -- Create structure
    os.mkdir(path.join(package_root, "DEBIAN"))
    os.mkdir(path.join(package_root, "usr", "local", "bin"))
    os.mkdir(path.join(package_root, "usr", "share", "applications"))
    os.mkdir(path.join(package_root, "usr", "share", "icons", "hicolor", "256x256", "apps"))

    -- Create control file
    local control_content = string.format([[
Package: %s
Version: %s
Section: utils
Priority: optional
Architecture: %s
Maintainer: %s <sast-team@example.com>
Description: %s
]], metadata.name, metadata.version, arch, metadata.author, metadata.description)
    io.writefile(path.join(package_root, "DEBIAN", "control"), control_content)

    -- Copy executable
    os.cp(target_file, path.join(package_root, "usr", "local", "bin", metadata.name))

    -- Copy desktop file
    os.cp(path.join(os.projectdir(), "distrib", "sast-readium.desktop"), path.join(package_root, "usr", "share", "applications"))

    -- Copy icon
    os.cp(path.join(os.projectdir(), "assets", "images", "logo.png"), path.join(package_root, "usr", "share", "icons", "hicolor", "256x256", "apps", metadata.name .. ".png"))

    -- Build the package
    os.execv("dpkg-deb", {"--build", package_root, output_dir})

    cprint("${green}DEB package created: %s_${clear}", path.filename(package_root) .. ".deb")

    -- Cleanup
    os.rm(temp_dir)
end

-- Generate SHA256 checksum
function generate_checksum(file_path)
    if not os.isfile(file_path) then
        return
    end

    local checksum_file = file_path .. ".sha256"
    local hash = hash.sha256(io.readfile(file_path, {encoding = "binary"}))
    local filename = path.filename(file_path)

    io.writefile(checksum_file, string.format("%s  %s\\n", hash, filename))

    if not has_config("quiet") then
        cprint("${dim}Generated SHA256: %s${clear}", checksum_file)
    end
end

-- Main packaging function
function create_packages(target)
    if not has_config("enable_packaging") then
        return
    end

    cprint("${bright}=== Creating Packages ===${clear}")

    local format = get_config("package_format")
    local formats = {}

    if format == "auto" then
        formats = get_default_package_formats()
    else
        table.insert(formats, format)
    end

    for _, fmt in ipairs(formats) do
        if fmt == "nsis" then
            create_nsis_package(target)
        elseif fmt == "zip" then
            create_zip_package(target)
        elseif fmt == "targz" then
            create_targz_package(target)
        elseif fmt == "dmg" then
            create_dmg_package(target)
        elseif fmt == "deb" then
            create_deb_package(target)
        elseif fmt == "rpm" or fmt == "appimage" then
            cprint("${yellow}%s packaging not yet implemented${clear}", fmt:upper())
        end
    end

    cprint("${bright}=== Packaging Complete ===${clear}")
    cprint("${green}Packages available in: %s${clear}", path.join(os.projectdir(), "package"))
end
