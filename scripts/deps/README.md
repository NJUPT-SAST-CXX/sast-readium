# Dependency Management Scripts

Scripts for managing and analyzing project dependencies.

## Scripts

### check-msys2-deps.sh

Check and install MSYS2 build dependencies.

```bash
# Check all dependencies
./scripts/deps/check-msys2-deps.sh

# Install missing dependencies
./scripts/deps/check-msys2-deps.sh -i

# Verbose output
./scripts/deps/check-msys2-deps.sh -v
```

### analyze-dependencies.ps1

Analyze DLL dependencies for Windows builds.

```powershell
# Analyze default executable
.\scripts\deps\analyze-dependencies.ps1

# Analyze specific executable
.\scripts\deps\analyze-dependencies.ps1 -Executable "build\Release-MSYS2\app\sast-readium.exe"

# Output as JSON
.\scripts\deps\analyze-dependencies.ps1 -Format JSON

# Check for missing dependencies
.\scripts\deps\analyze-dependencies.ps1 -CheckMissing
```

### bundle-dlls.sh

Bundle required DLLs for MSYS2 distribution.

```bash
# Bundle DLLs for default build
./scripts/deps/bundle-dlls.sh

# Bundle for specific build directory
./scripts/deps/bundle-dlls.sh build/Release-MSYS2

# Create ZIP archive
./scripts/deps/bundle-dlls.sh --zip
```
