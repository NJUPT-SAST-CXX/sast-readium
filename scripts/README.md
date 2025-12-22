# Scripts Directory

Utility scripts for building, testing, packaging, and developing SAST Readium.

## Directory Structure

```
scripts/
├── common.sh              # Shared bash utilities
├── common.ps1             # Shared PowerShell utilities
├── build/                 # Build scripts
│   ├── build-msys2.sh
│   ├── setup-build-environment.ps1
│   └── legacy/            # Deprecated scripts
├── dev/                   # Development environment
│   ├── clangd-config.sh
│   ├── clangd-config.ps1
│   └── run-clang-tidy.ps1
├── deps/                  # Dependency management
│   ├── check-msys2-deps.sh
│   ├── analyze-dependencies.ps1
│   └── bundle-dlls.sh
├── test/                  # Testing scripts
│   ├── run-tests.ps1
│   ├── run-tests.sh
│   ├── run-tests.bat
│   └── ...
├── package/               # Packaging scripts
│   ├── package.ps1
│   ├── package.sh
│   └── ...
└── debug/                 # Debugging scripts
    ├── debug-freeze.gdb
    └── verify-freeze-fix.gdb
```

## Quick Start

### Windows with vcpkg

```powershell
# Configure build environment (auto-detect compiler)
.\scripts\build\setup-build-environment.ps1

# Build
cmake --build build\Debug-Windows --config Debug

# Update clangd config
.\scripts\dev\clangd-config.ps1 -Auto
```

### Windows MSYS2

```bash
# Build with dependency installation
./scripts/build/build-msys2.sh --type Debug --install-deps

# Update clangd config
./scripts/dev/clangd-config.sh --auto
```

### Linux/macOS

```bash
# Configure and build
cmake --preset Debug-Unix
cmake --build build/Debug-Unix

# Update clangd config
./scripts/dev/clangd-config.sh --auto
```

## Categories

### Build Scripts (`build/`)

Scripts for building the project on different platforms.

| Script | Platform | Description |
|--------|----------|-------------|
| `build-unix.sh` | Linux/macOS | Full build automation with deps, coverage |
| `build-msys2.sh` | MSYS2 | Full build automation with deps, LTO, UPX |
| `setup-build-environment.ps1` | Windows | Universal vcpkg environment setup |

See [build/README.md](build/README.md) for details.

### Development Scripts (`dev/`)

Scripts for configuring the development environment.

| Script | Platform | Description |
|--------|----------|-------------|
| `clangd-config.sh` | Unix/MSYS2 | Configure clangd language server |
| `clangd-config.ps1` | Windows | Configure clangd language server |
| `run-clang-tidy.sh` | Unix/MSYS2 | Run comprehensive clang-tidy analysis |
| `run-clang-tidy.ps1` | Windows | Run comprehensive clang-tidy analysis |

See [dev/README.md](dev/README.md) for details.

### Dependency Scripts (`deps/`)

Scripts for managing project dependencies.

| Script | Platform | Description |
|--------|----------|-------------|
| `check-msys2-deps.sh` | MSYS2 | Check/install MSYS2 dependencies |
| `analyze-dependencies.ps1` | Windows | Analyze DLL dependencies |
| `bundle-dlls.sh` | MSYS2 | Bundle DLLs for distribution |

See [deps/README.md](deps/README.md) for details.

### Testing Scripts (`test/`)

Scripts for running tests.

| Script | Platform | Description |
|--------|----------|-------------|
| `run-tests.ps1` | Windows | PowerShell test runner with coverage |
| `run-tests.sh` | MSYS2 | Bash test runner |
| `run-tests-unix.sh` | Linux/macOS | Full test runner with coverage |
| `run-tests.bat` | Windows | Batch test runner |
| `run-qgraphics-tests.py` | All | Python QGraphics test runner |

See [test/README.md](test/README.md) for details.

### Packaging Scripts (`package/`)

Scripts for creating distribution packages.

| Script | Platform | Description |
|--------|----------|-------------|
| `package.ps1` | Windows | Create MSI, NSIS, ZIP packages |
| `package.sh` | Linux/macOS | Create deb, rpm, AppImage, dmg |
| `package-msys2.sh` | MSYS2 | Create MSYS2-specific packages |
| `verify-package.ps1` | Windows | Verify package integrity |
| `verify-package.sh` | Linux/macOS | Verify package integrity |

See [package/README.md](package/README.md) for details.

### Debugging Scripts (`debug/`)

GDB scripts for debugging freezes and deadlocks.

| Script | Description |
|--------|-------------|
| `debug-freeze.gdb` | Diagnose application freezes |
| `verify-freeze-fix.gdb` | Verify freeze fix implementation |

See [debug/README.md](debug/README.md) for details.

## Shared Utilities

### common.sh (Bash)

Source at the beginning of your bash script:

```bash
source "$(dirname "${BASH_SOURCE[0]}")/common.sh"
```

Provides: `log_success`, `log_error`, `log_warning`, `log_info`, `detect_project_root`, `detect_msys2_root`, etc.

### common.ps1 (PowerShell)

Dot-source at the beginning of your PowerShell script:

```powershell
. "$PSScriptRoot\common.ps1"
```

Provides: `Write-Success`, `Write-Error-Custom`, `Find-MSYS2Root`, `Find-VcpkgRoot`, etc.

## Build Presets

The project supports 6 essential CMake presets:

| Preset | Platform | Package Manager |
|--------|----------|-----------------|
| Debug-Unix / Release-Unix | Linux/macOS | System packages |
| Debug-Windows / Release-Windows | Windows | vcpkg |
| Debug-MSYS2 / Release-MSYS2 | MSYS2 | pacman |

For more information, see the [Build System Documentation](../docs/build-systems/).
