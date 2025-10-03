# clangd Indexing Issues - Troubleshooting Guide

This document provides comprehensive troubleshooting steps for clangd indexing issues in the SAST Readium project.

## Quick Diagnosis

If clangd cannot find or build the index for the codebase, follow these steps:

### 1. Verify clangd Installation

```bash
# Check clangd version
clangd --version

# Expected output: clangd version 11.0.0 or higher
```

**Required:** clangd version 11.0.0+ (version 21.1.1 recommended for best compatibility)

### 2. Verify Compilation Database Exists

```bash
# Windows PowerShell
Test-Path "build/Debug-MSYS2/compile_commands.json"

# Linux/macOS
ls -la build/Debug-MSYS2/compile_commands.json
```

**Expected:** File should exist and be non-empty (typically 100KB+)

### 3. Verify .clangd Configuration

```bash
# Check .clangd file exists in project root
cat .clangd  # Linux/macOS
Get-Content .clangd  # Windows PowerShell
```

**Expected:** File should exist with correct syntax (see below)

### 4. Test clangd Parsing

```bash
# Test if clangd can parse a file
clangd --check="app/MainWindow.cpp"
```

**Expected output:**

```
I[timestamp] All checks completed, 0 errors
```

**Warning signs:**

```
config warning at .clangd:XX:X: Unknown Config key 'CompilationDatabase'
```

## Common Issues and Solutions

### Issue 1: "Unknown Config key 'CompilationDatabase'" Warning

**Symptom:** clangd logs show configuration warnings about `CompilationDatabase`

**Root Cause:** Incorrect `.clangd` configuration syntax. In clangd 11+, `CompilationDatabase` must be under the `CompileFlags` section, not as a top-level key.

**Solution:**

**❌ Incorrect (Old Syntax):**

```yaml
CompileFlags:
  Add:
    - -std=c++20
  Remove:
    - -W*

CompilationDatabase: build/Debug-MSYS2  # WRONG: Top-level key
```

**✅ Correct (New Syntax):**

```yaml
CompileFlags:
  Add:
    - -std=c++20
  Remove:
    - -W*
  CompilationDatabase: build/Debug-MSYS2  # CORRECT: Under CompileFlags
```

**Fix:**

1. Edit `.clangd` file in project root
2. Move `CompilationDatabase` line under `CompileFlags` section (indent with 2 spaces)
3. Restart your IDE/language server

**Automated Fix:**

```bash
# Regenerate .clangd with correct syntax
cmake --preset=Debug-MSYS2  # Regenerates .clangd automatically
```

### Issue 2: compile_commands.json Not Found

**Symptom:** clangd cannot find compilation database

**Root Cause:** CMake hasn't generated the compilation database, or it's in the wrong location

**Solution:**

1. **Ensure CMAKE_EXPORT_COMPILE_COMMANDS is enabled:**
   - This is automatically enabled in `cmake/ProjectConfig.cmake`
   - Verify: `grep CMAKE_EXPORT_COMPILE_COMMANDS cmake/ProjectConfig.cmake`

2. **Regenerate build configuration:**

   ```bash
   # Clean and reconfigure
   rm -rf build/Debug-MSYS2  # Linux/macOS
   Remove-Item -Recurse -Force build/Debug-MSYS2  # Windows PowerShell
   
   cmake --preset=Debug-MSYS2
   cmake --build --preset=Debug-MSYS2
   ```

3. **Verify generation:**

   ```bash
   # Check file exists and is recent
   ls -lh build/Debug-MSYS2/compile_commands.json
   ```

### Issue 3: clangd Index Cache Corruption

**Symptom:** clangd indexing is slow, incomplete, or produces incorrect results

**Root Cause:** Corrupted index cache files

**Solution:**

1. **Locate clangd cache directory:**
   - **Windows:** `%LocalAppData%\clangd\index`
   - **macOS:** `~/Library/Preferences/clangd/index`
   - **Linux:** `~/.cache/clangd/index`

2. **Clear cache:**

   ```bash
   # Windows PowerShell
   Remove-Item -Recurse -Force "$env:LOCALAPPDATA\clangd\index"
   
   # Linux/macOS
   rm -rf ~/.cache/clangd/index
   # or
   rm -rf ~/Library/Preferences/clangd/index  # macOS
   ```

3. **Restart IDE and allow reindexing:**
   - Close all editor windows
   - Restart IDE
   - Open a source file and wait for indexing to complete

### Issue 4: IDE Not Picking Up .clangd Changes

**Symptom:** Changes to `.clangd` file don't take effect

**Root Cause:** Language server needs to be restarted

**Solution:**

**VS Code:**

1. Open Command Palette (Ctrl+Shift+P / Cmd+Shift+P)
2. Run: "clangd: Restart language server"

**CLion:**

1. File → Invalidate Caches / Restart
2. Select "Invalidate and Restart"

**Neovim/Vim:**

```vim
:LspRestart
```

**Generic:**

- Close and reopen your IDE
- Or kill the clangd process manually

### Issue 5: C++20 Module Flags Causing Issues

**Symptom:** clangd shows errors related to C++20 modules

**Root Cause:** CMake 3.28+ automatically adds C++20 module flags that clangd doesn't fully support

**Solution:**

The `.clangd` configuration automatically removes these flags:

```yaml
CompileFlags:
  Remove:
    - -fmodules-ts
    - -fmodule-mapper=*
    - -fdeps-format=*
```

If issues persist:

1. Verify these flags are in your `.clangd` file
2. Regenerate configuration: `cmake --preset=Debug-MSYS2`
3. Restart language server

## Verification Steps

After applying fixes, verify clangd is working correctly:

### 1. Check Configuration Loading

```bash
clangd --check="app/MainWindow.cpp" 2>&1 | grep -E "config|Config"
```

**Expected:** No warnings about unknown config keys

### 2. Verify Indexing Performance

```bash
clangd --check="app/MainWindow.cpp" 2>&1 | grep -E "Built preamble|All checks"
```

**Expected output:**

```
I[timestamp] Built preamble of size XXXXXX for file ... in X.XX seconds
I[timestamp] All checks completed, 0 errors
```

**Performance benchmarks:**

- Preamble build: 5-10 seconds (first time), <1 second (cached)
- Full check: 10-15 seconds (first time), 1-2 seconds (cached)

### 3. Test IDE Features

Open a source file and verify:

- ✅ Code completion works (Ctrl+Space)
- ✅ Go to definition works (F12 / Cmd+Click)
- ✅ Find references works (Shift+F12)
- ✅ Hover shows type information
- ✅ Diagnostics appear for errors

## Advanced Troubleshooting

### Enable clangd Logging

**VS Code:**
Add to `settings.json`:

```json
{
  "clangd.arguments": [
    "--log=verbose",
    "--pretty"
  ]
}
```

**Command Line:**

```bash
clangd --log=verbose --pretty < input.txt > output.txt 2> clangd.log
```

### Check Compilation Database Content

```bash
# View first entry
jq '.[0]' build/Debug-MSYS2/compile_commands.json

# Count entries
jq 'length' build/Debug-MSYS2/compile_commands.json
```

**Expected:** Should contain entries for all source files in the project

### Manual Index Building

```bash
# Build index manually
clangd-indexer --executor=all-TUs build/Debug-MSYS2/compile_commands.json > index.yaml
```

## Prevention

### Automatic Configuration Updates

The project automatically maintains `.clangd` configuration:

1. **During CMake configuration:**

   ```bash
   cmake --preset=Debug-MSYS2  # Auto-generates .clangd
   ```

2. **Manual update scripts:**

   ```bash
   # Auto-detect and update
   ./scripts/clangd-config.sh --auto    # Linux/macOS
   .\scripts\clangd-config.ps1 -Auto    # Windows
   ```

### Best Practices

1. **Always use CMake presets** - Ensures correct configuration
2. **Restart language server after configuration changes**
3. **Clear cache when switching branches** - Prevents stale index issues
4. **Keep clangd updated** - Newer versions have better C++20 support
5. **Use system packages when possible** - Faster than vcpkg builds

## Getting Help

If issues persist after following this guide:

1. **Check clangd logs** - Enable verbose logging
2. **Verify build system** - Ensure project builds successfully
3. **Test with minimal example** - Create a simple test file
4. **Report issue** - Include:
   - clangd version (`clangd --version`)
   - OS and IDE
   - `.clangd` configuration
   - Relevant log excerpts

## References

- [clangd Official Documentation](https://clangd.llvm.org/)
- [clangd Configuration Reference](https://clangd.llvm.org/config)
- [Project clangd Setup Guide](../setup/clangd-setup.md)
- [Build Artifact Management](../setup/build-artifact-management.md)
