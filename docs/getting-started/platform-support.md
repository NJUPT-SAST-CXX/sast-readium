# Platform Support Matrix

This document outlines the comprehensive platform support for SAST Readium, including native and cross-compilation targets.

## Supported Platforms

### ✅ Fully Supported (CI Tested)

| Platform    | Architecture          | Build Method | Package Manager          | CI Status      |
| ----------- | --------------------- | ------------ | ------------------------ | -------------- |
| **Linux**   | x86_64                | Native       | System packages (apt)    | ✅ Primary     |
| **macOS**   | x86_64 (Intel)        | Native       | System packages (brew)   | ✅ Primary     |
| **macOS**   | ARM64 (Apple Silicon) | Native       | System packages (brew)   | ✅ Primary     |
| **Windows** | x86_64                | Native       | vcpkg                    | ✅ Primary     |
| **Windows** | x86_64                | MSYS2        | System packages (pacman) | ✅ Alternative |
| **Windows** | i686 (32-bit)         | MSYS2        | System packages (pacman) | ✅ Legacy      |

### 🔄 Cross-Compilation Supported (On-Demand)

| Platform    | Architecture    | Build Method  | Package Manager | CI Status    |
| ----------- | --------------- | ------------- | --------------- | ------------ |
| **Linux**   | ARM64 (aarch64) | Cross-compile | System packages | 🔄 On-demand |
| **Windows** | ARM64           | Cross-compile | vcpkg           | 🔄 On-demand |

### 🚧 Planned Support

| Platform    | Architecture | Build Method  | Package Manager | Status     |
| ----------- | ------------ | ------------- | --------------- | ---------- |
| **FreeBSD** | x86_64       | Native        | pkg             | 🚧 Planned |
| **Linux**   | RISC-V       | Cross-compile | System packages | 🚧 Future  |

## Build Configuration Details

### Linux x86_64 (Primary)

```bash
# System dependencies
sudo apt install qt6-base-dev qt6-svg-dev qt6-tools-dev qt6-speech-dev libpoppler-qt6-dev

# Build
cmake --preset=Release-Unix
cmake --build --preset=Release-Unix
```

**Advantages:**

- Fastest build times
- Excellent package ecosystem
- Native performance
- Easy debugging

### Linux ARM64 (Cross-compilation)

```bash
# Cross-compilation setup
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
sudo dpkg --add-architecture arm64
sudo apt install qt6-base-dev:arm64 qt6-svg-dev:arm64 libpoppler-qt6-dev:arm64

# Build
cmake --preset=Release-Linux-ARM64
cmake --build --preset=Release-Linux-ARM64
```

**Use Cases:**

- Raspberry Pi deployment
- ARM-based servers
- Embedded Linux systems

### macOS Intel (x86_64)

```bash
# System dependencies
brew install qt@6 poppler-qt5 cmake ninja

# Build
cmake --preset=Release-Unix
cmake --build --preset=Release-Unix
```

**Advantages:**

- Native Intel Mac support
- Homebrew ecosystem
- Development on older Macs

### macOS Apple Silicon (ARM64)

```bash
# System dependencies (same as Intel)
brew install qt@6 poppler-qt5 cmake ninja

# Build (automatically detects ARM64)
cmake --preset=Release-Unix
cmake --build --preset=Release-Unix
```

**Advantages:**

- Native Apple Silicon performance
- Universal binary support
- Modern Mac compatibility

### Windows x86_64 (vcpkg)

```bash
# Setup vcpkg
git clone https://github.com/Microsoft/vcpkg.git
.\vcpkg\bootstrap-vcpkg.bat

# Build
cmake --preset=Release-Windows
cmake --build --preset=Release-Windows
```

**Use Cases:**

- Visual Studio development
- Windows-specific features
- MSVC compiler optimizations

### Windows x86_64 (MSYS2 - Recommended)

```bash
# System dependencies
pacman -S mingw-w64-x86_64-qt6-base mingw-w64-x86_64-qt6-svg mingw-w64-x86_64-qt6-speech mingw-w64-x86_64-poppler-qt6

# Build
cmake --preset=Release-MSYS2
cmake --build --preset=Release-MSYS2
```

**Advantages:**

- Unix-like development experience
- Faster builds than vcpkg
- Better package management
- Native Windows binaries

### Windows ARM64 (Cross-compilation)

```bash
# Setup vcpkg with ARM64 triplet
cmake --preset=Release-Windows-ARM64
cmake --build --preset=Release-Windows-ARM64
```

**Use Cases:**

- Windows on ARM devices
- Surface Pro X
- ARM-based Windows laptops

## CI/CD Workflow Strategy

### Primary Builds (Always Run)

- **Linux x86_64** - System packages (fastest)
- **macOS Intel** - System packages
- **macOS Apple Silicon** - System packages
- **Windows x86_64** - vcpkg (most compatible)
- **Windows MSYS2** - System packages (alternative)

### On-Demand Builds (Manual Trigger or Tags)

- **Linux ARM64** - Cross-compilation
- **Windows ARM64** - Cross-compilation

### Trigger Conditions

**Primary builds run on:**

- All push events
- All pull requests
- Manual workflow dispatch

**ARM64 builds run on:**

- Manual workflow dispatch with `build_all_platforms=true`
- Git tags (releases)
- Push to release branches

## Performance Characteristics

### Build Time Comparison (Approximate)

| Platform      | Method | Clean Build | Incremental | Cache Hit |
| ------------- | ------ | ----------- | ----------- | --------- |
| Linux x86_64  | System | 3-5 min     | 30-60 sec   | 1-2 min   |
| macOS x86_64  | System | 4-6 min     | 45-90 sec   | 1-3 min   |
| macOS ARM64   | System | 3-4 min     | 30-45 sec   | 1-2 min   |
| Windows       | vcpkg  | 15-25 min   | 2-5 min     | 5-10 min  |
| Windows       | MSYS2  | 5-8 min     | 1-2 min     | 2-3 min   |
| Linux ARM64   | Cross  | 8-12 min    | 2-4 min     | 3-5 min   |
| Windows ARM64 | Cross  | 20-30 min   | 3-6 min     | 8-12 min  |

### Resource Usage

| Platform      | CPU Cores | Memory | Disk Space |
| ------------- | --------- | ------ | ---------- |
| Linux x86_64  | 2-4       | 2-4 GB | 1-2 GB     |
| macOS         | 2-4       | 3-6 GB | 2-3 GB     |
| Windows vcpkg | 2-4       | 4-8 GB | 5-10 GB    |
| Windows MSYS2 | 2-4       | 2-4 GB | 2-3 GB     |
| Cross-compile | 2-4       | 3-6 GB | 3-5 GB     |

## Deployment Targets

### Desktop Distributions

- **Linux**: AppImage, .deb, .rpm, Flatpak
- **macOS**: .dmg, .app bundle, Homebrew
- **Windows**: .msi installer, portable .exe, Microsoft Store

### Server/Embedded

- **Linux ARM64**: Docker containers, system packages
- **Raspberry Pi**: Native ARM64 builds
- **IoT devices**: Minimal Qt builds

## Development Recommendations

### For Contributors

1. **Primary development**: Use your native platform with system packages
2. **Cross-platform testing**: Use MSYS2 on Windows for Unix-like experience
3. **Release testing**: Test on all primary platforms before release

### For CI/CD

1. **Fast feedback**: Prioritize system package builds
2. **Comprehensive testing**: Include vcpkg builds for compatibility
3. **Release preparation**: Build all platforms including ARM64

### For Deployment

1. **Linux**: Prefer system packages, fallback to AppImage
2. **macOS**: Universal binaries for Intel + Apple Silicon
3. **Windows**: MSYS2 builds for performance, vcpkg for compatibility

## Troubleshooting

### Common Issues

**Linux ARM64 Cross-compilation:**

```bash
# Missing cross-compiler
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# Missing ARM64 libraries
sudo dpkg --add-architecture arm64
sudo apt update
```

**macOS Universal Binaries:**

```bash
# Build for both architectures
cmake --preset=Release-Unix -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
```

**Windows ARM64:**

```bash
# Ensure ARM64 toolchain
cmake --preset=Release-Windows-ARM64 -A ARM64
```

### Platform-Specific Notes

- **Linux**: Requires recent Qt6 packages (Ubuntu 22.04+)
- **macOS**: Requires Xcode Command Line Tools
- **Windows**: MSYS2 provides better Unix compatibility than vcpkg
- **ARM64**: Cross-compilation requires careful dependency management
