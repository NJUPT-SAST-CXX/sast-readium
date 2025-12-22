# Development Environment Scripts

Scripts for configuring the development environment and running code quality tools.

## Scripts

### clangd-config.sh / clangd-config.ps1

Configure clangd language server for IDE integration.

**Bash (Unix/MSYS2):**

```bash
# Auto-detect and use the most recent build directory
./scripts/dev/clangd-config.sh --auto

# Use a specific build directory
./scripts/dev/clangd-config.sh build/Debug-Unix

# List all available build directories
./scripts/dev/clangd-config.sh --list

# Enable verbose output
./scripts/dev/clangd-config.sh --auto --verbose
```

**PowerShell (Windows):**

```powershell
# Auto-detect build directory
.\scripts\dev\clangd-config.ps1 -Auto

# Use a specific build directory
.\scripts\dev\clangd-config.ps1 -BuildDir "build\Debug-Windows"

# Force update even if current config is valid
.\scripts\dev\clangd-config.ps1 -Auto -Force
```

### run-clang-tidy.ps1 / run-clang-tidy.sh

Cross-platform clang-tidy runner for code quality analysis.

**PowerShell (Windows):**

```powershell
# Run analysis on all files
.\scripts\dev\run-clang-tidy.ps1

# Run with automatic fixes
.\scripts\dev\run-clang-tidy.ps1 -Fix

# Specify custom build directory
.\scripts\dev\run-clang-tidy.ps1 -BuildDir "build/Release-MSYS2"
```

**Bash (Unix/MSYS2):**

```bash
# Run analysis
./scripts/dev/run-clang-tidy.sh

# Run with fixes
./scripts/dev/run-clang-tidy.sh --fix

# Custom build directory
./scripts/dev/run-clang-tidy.sh -b build/Release-Unix -v
```

**Options:**

- `-c, --clang-tidy`: Path to clang-tidy (auto-detected)
- `-b, --build-dir`: Build directory with compile_commands.json
- `-o, --output`: Output file for results
- `-f, --fix`: Apply suggested fixes
- `--fix-errors`: Apply fixes for errors only
- `-v, --verbose`: Verbose output
