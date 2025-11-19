# SAST Readium 打包指南

本文档介绍如何为不同平台创建 SAST Readium 的安装包。

## 支持的平台与格式

### Windows

- **NSIS 安装器** (.exe) - 标准 Windows 安装程序
- **WiX MSI** (.msi) - Microsoft Installer 格式（MSVC 构建）
- **ZIP 便携版** (.zip) - 免安装压缩包

### Linux

- **DEB 包** (.deb) - Debian/Ubuntu 系列
- **RPM 包** (.rpm) - RedHat/Fedora/openSUSE 系列
- **AppImage** - 通用可执行格式
- **TGZ 归档** (.tar.gz) - 源代码风格打包

### macOS

- **DMG 镜像** (.dmg) - 标准 macOS 安装格式
- **TGZ 归档** (.tar.gz) - 通用压缩格式

## 使用 CMake 打包

### 快速打包

```bash
# 配置（启用打包）
cmake -B build -DCMAKE_BUILD_TYPE=Release \
      -DENABLE_PACKAGING=ON \
      -DSAST_ENABLE_LTO=ON \
      -DSAST_ENABLE_HARDENING=ON

# 构建
cmake --build build --config Release

# 生成所有包
cd build
cpack -C Release
```

### 指定包格式

```bash
# 仅生成 ZIP 便携包
cpack -C Release -G ZIP

# 仅生成 NSIS 安装器（Windows）
cpack -C Release -G NSIS

# 仅生成 DEB 包（Linux）
cpack -C Release -G DEB

# 生成多个格式
cpack -C Release -G "ZIP;NSIS"
```

### 高级选项

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release \
      -DENABLE_PACKAGING=ON \
      -DPACKAGE_PORTABLE=ON \
      -DPACKAGE_INSTALLER=ON \
      -DPACKAGING_MINIMAL=ON \          # 最小化部署
      -DPACKAGING_STRIP_DEBUG=ON \      # 剥离调试符号
      -DSAST_ENABLE_LTO=ON              # 启用 LTO 优化
```

## 使用 XMake 打包

### 快速打包

```bash
# 配置
xmake f -m release --enable_packaging=y --enable_lto=y

# 构建（自动打包）
xmake
```

打包会在构建完成后自动执行，生成平台默认格式。

### 指定包格式

```bash
# 仅生成 ZIP 便携包
xmake f -m release --enable_packaging=y --package_format=zip
xmake

# 生成 NSIS 安装器（Windows）
xmake f -m release --enable_packaging=y --package_format=nsis
xmake

# 使用默认格式（auto 自动选择）
xmake f -m release --enable_packaging=y --package_format=auto
xmake
```

### 禁用自动打包

```bash
xmake f -m release --enable_packaging=n
xmake
```

## 统一打包脚本

项目提供了跨平台统一脚本：

### Linux/macOS

```bash
# CMake
BUILD_TYPE=Release BUILD_SYSTEM=cmake ./scripts/create-packages.sh

# XMake
BUILD_TYPE=Release BUILD_SYSTEM=xmake ./scripts/create-packages.sh
```

### Windows (PowerShell)

```powershell
# CMake
.\scripts\package.ps1 -BuildType Release -BuildSystem cmake

# XMake
.\scripts\package.ps1 -BuildType Release -BuildSystem xmake
```

## 平台特定说明

### Windows

#### NSIS 安装器

- 需要安装 [NSIS](https://nsis.sourceforge.io/)
- 生成带安装向导的 .exe 文件
- 支持桌面快捷方式、开始菜单、卸载程序
- 可选 PDF 文件关联

#### WiX MSI（MSVC 构建）

- 需要安装 [WiX Toolset](https://wixtoolset.org/)
- 生成标准 Windows Installer (.msi)
- 企业环境友好
- 支持升级和修复

#### 代码签名（可选）

```bash
# 设置环境变量
export SIGNTOOL_PATH="C:/Program Files (x86)/Windows Kits/10/bin/x64/signtool.exe"

# 打包时自动签名
cpack -C Release
```

### Linux

#### DEB 包

```bash
# 安装依赖
sudo apt install dpkg-dev

# 构建
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_PACKAGING=ON
cmake --build build
cd build
cpack -G DEB
```

#### RPM 包

```bash
# 安装依赖
sudo dnf install rpm-build

# 构建
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_PACKAGING=ON
cmake --build build
cd build
cpack -G RPM
```

#### AppImage

```bash
# 安装 linuxdeploy
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
chmod +x linuxdeploy-x86_64.AppImage
sudo mv linuxdeploy-x86_64.AppImage /usr/local/bin/linuxdeploy

# 安装 Qt 插件
wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
chmod +x linuxdeploy-plugin-qt-x86_64.AppImage
sudo mv linuxdeploy-plugin-qt-x86_64.AppImage /usr/local/bin/linuxdeploy-plugin-qt

# 构建 AppImage
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_PACKAGING=ON
cmake --build build
cd build
cpack -G External
```

### macOS

#### DMG 镜像

```bash
# 构建
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_PACKAGING=ON
cmake --build build
cd build
cpack -G DragNDrop
```

#### 代码签名与公证

```bash
# 设置环境变量
export CODESIGN_IDENTITY="Developer ID Application: YourName (TEAMID)"

# 构建并签名
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_PACKAGING=ON
cmake --build build
cd build
cpack -G DragNDrop

# 公证（需要 Apple Developer 账号）
xcrun notarytool submit SASTReadium-*.dmg --keychain-profile "notarytool-profile" --wait
xcrun stapler staple SASTReadium-*.dmg
```

## 包内容验证

### 检查包内容

#### Windows (ZIP)

```powershell
Expand-Archive -Path SASTReadium-*.zip -DestinationPath temp
tree /F temp
```

#### Linux (TGZ)

```bash
tar -tzf SASTReadium-*.tar.gz
```

#### Linux (DEB)

```bash
dpkg -c SASTReadium-*.deb
```

### 校验和验证

所有包都会自动生成 SHA256 校验和文件：

```bash
# 验证单个包
sha256sum -c SASTReadium-0.1.0.0-Windows-x64-Release.zip.sha256

# 验证所有包
sha256sum -c SHA256SUMS
```

## 包命名规范

格式：`SASTReadium-<版本>-<平台>-<架构>-<构建类型>.<扩展名>`

示例：

- `SASTReadium-0.1.0.0-Windows-x64-Release.zip`
- `SASTReadium-0.1.0.0-Linux-x86_64-Release.deb`
- `SASTReadium-0.1.0.0-macOS-arm64-Release.dmg`

## 包输出目录

- **CMake**: `build/package/` 或 `package/`
- **XMake**: `package/`

## 依赖打包策略（跨平台概览）

不同平台在依赖处理上的策略略有差异：

- **Windows（MSVC + MSYS2）**
  - 使用 CMake `RUNTIME_DEPENDENCIES` 与 `InstallRequiredSystemLibraries` 自动收集并安装运行时 DLL。
  - MSVC 构建通过 WiX MSI 打包 VC++ 运行库。
  - MSYS2 构建会额外复制 `libgcc_s_seh-1.dll`、`libstdc++-6.dll`、`libwinpthread-1.dll` 以及 Poppler/ spdlog 等 DLL，并使用 `windeployqt` 部署 Qt。
  - 可选的最小化打包选项会移除头文件、静态库、调试符号和多余的 Qt 插件以减小体积。

- **Linux（DEB/RPM + AppImage）**
  - DEB/RPM 采用“系统包为主”的策略：
    - DEB 依赖默认包含：`libc6, libqt6core6, libqt6gui6, libqt6widgets6, libqt6svg6, libpoppler-qt6-3`。
    - RPM 使用对应的 Qt6/Poppler 包组，适配发行版的依赖解析。
  - AppImage 采用 `linuxdeploy` + `linuxdeploy-plugin-qt`（若可用）将 Qt 框架和插件一起打包，使其在绝大多数发行版上开箱即用。

- **macOS（.app + DMG/PKG）**
  - 可执行文件构建为标准 `MACOSX_BUNDLE` `.app`，并使用自定义 `Info.plist` 声明 PDF 文档类型。
  - DMG 使用 `DragNDrop` 生成，PKG 使用 CPack `productbuild` 生成安装包。
  - 若设置了 `CODESIGN_IDENTITY`，安装阶段会自动对 `.app` 进行签名，为后续公证与分发做准备。

在所有平台上，`PACKAGING_MINIMAL`、`PACKAGING_STRIP_DEBUG`、`PACKAGING_AGGRESSIVE_CLEANUP` 组合用于控制依赖的“打包多少”和“删多少”，在可用性和包体积之间取得平衡。

## CI/CD 集成

### GitHub Actions 示例

```yaml
name: Package

on:
  release:
    types: [created]

jobs:
  package-windows:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v3
      - name: Build and Package
        run: |
          cmake -B build -DENABLE_PACKAGING=ON
          cmake --build build --config Release
          cd build && cpack -C Release
      - name: Upload Packages
        uses: actions/upload-artifact@v3
        with:
          name: windows-packages
          path: build/package/*

  package-linux:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Install Dependencies
        run: sudo apt install qt6-base-dev libpoppler-qt6-dev
      - name: Build and Package
        run: |
          cmake -B build -DENABLE_PACKAGING=ON
          cmake --build build
          cd build && cpack
      - name: Upload Packages
        uses: actions/upload-artifact@v3
        with:
          name: linux-packages
          path: build/package/*

  package-macos:
    runs-on: macos-latest
    steps:
      - uses: actions/checkout@v3
      - name: Build and Package
        run: |
          cmake -B build -DENABLE_PACKAGING=ON
          cmake --build build
          cd build && cpack -G DragNDrop
      - name: Upload Packages
        uses: actions/upload-artifact@v3
        with:
          name: macos-packages
          path: build/package/*
```

## 故障排查

### 问题：缺少 Qt 插件

**Windows:**

```bash
# 手动运行 windeployqt
windeployqt --release path/to/sast-readium.exe
```

**Linux:**

```bash
# 设置 LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/usr/local/qt6/lib:$LD_LIBRARY_PATH
```

### 问题：NSIS 错误

确保 NSIS 在 PATH 中：

```bash
# Windows
where makensis

# 如果找不到，添加到 PATH
set PATH=%PATH%;C:\Program Files (x86)\NSIS
```

### 问题：DEB/RPM 依赖缺失

编辑 `cmake/Packaging.cmake` 中的依赖列表：

```cmake
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libqt6core6, libqt6gui6, libqt6widgets6, libpoppler-qt6-3")
```

## 最佳实践

1. **Release 构建时启用 LTO**：

   ```bash
   -DSAST_ENABLE_LTO=ON
   ```

2. **启用加固标志**：

   ```bash
   -DSAST_ENABLE_HARDENING=ON
   ```

3. **最小化打包**：

   ```bash
   -DPACKAGING_MINIMAL=ON
   ```

4. **剥离调试符号**：

   ```bash
   -DPACKAGING_STRIP_DEBUG=ON
   ```

5. **验证所有包**：

   ```bash
   sha256sum -c SHA256SUMS
   ```

## 参考资料

- [CPack Documentation](https://cmake.org/cmake/help/latest/module/CPack.html)
- [XMake Package Management](https://xmake.io/#/package/remote_package)
- [NSIS Documentation](https://nsis.sourceforge.io/Docs/)
- [WiX Toolset](https://wixtoolset.org/documentation/)
- [AppImage Documentation](https://docs.appimage.org/)

---

*最后更新：2025-01-14*
