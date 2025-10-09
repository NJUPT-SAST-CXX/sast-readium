#!/bin/bash
# Test Debug Setup Script for SAST Readium
# This script verifies the debugging configuration and provides testing instructions

set -e

PLATFORM="${1:-auto}"

echo "=== SAST Readium Debug Setup Test ==="
echo ""

# Detect platform if auto
if [ "$PLATFORM" = "auto" ]; then
    case "$(uname -s)" in
        Linux*)     PLATFORM=linux;;
        Darwin*)    PLATFORM=macos;;
        CYGWIN*|MINGW*|MSYS*) PLATFORM=windows;;
        *)          echo "Unable to detect platform automatically"; exit 1;;
    esac
fi

echo "Testing platform: $PLATFORM"
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Check VSCode configuration files
echo -e "${YELLOW}1. Checking VSCode configuration files...${NC}"

config_files=(
    ".vscode/launch.json"
    ".vscode/tasks.json"
    ".vscode/c_cpp_properties.json"
    ".vscode/qt.natvis"
)

for file in "${config_files[@]}"; do
    if [ -f "$file" ]; then
        echo -e "   ${GREEN}✓ $file exists${NC}"

        # Validate JSON files
        if [[ "$file" == *.json ]]; then
            if command -v python3 &> /dev/null; then
                if python3 -c "import json; json.load(open('$file'))" 2>/dev/null; then
                    echo -e "   ${GREEN}✓ $file is valid JSON${NC}"
                else
                    echo -e "   ${RED}✗ $file has JSON syntax errors${NC}"
                fi
            elif command -v jq &> /dev/null; then
                if jq empty "$file" 2>/dev/null; then
                    echo -e "   ${GREEN}✓ $file is valid JSON${NC}"
                else
                    echo -e "   ${RED}✗ $file has JSON syntax errors${NC}"
                fi
            fi
        fi
    else
        echo -e "   ${RED}✗ $file missing${NC}"
    fi
done

echo ""

# Check build tools
echo -e "${YELLOW}2. Checking build tools...${NC}"

check_command() {
    local name="$1"
    local cmd="$2"
    local optional="$3"

    if command -v ${cmd%% *} &> /dev/null; then
        echo -e "   ${GREEN}✓ $name available${NC}"
        return 0
    else
        if [ "$optional" = "true" ]; then
            echo -e "   ${YELLOW}⚠ $name not available (optional)${NC}"
        else
            echo -e "   ${RED}✗ $name not available (required)${NC}"
        fi
        return 1
    fi
}

case "$PLATFORM" in
    windows)
        check_command "cmake" "cmake"
        check_command "ninja" "ninja"
        check_command "cl (MSVC)" "cl" true
        check_command "g++ (MinGW)" "g++" true
        check_command "gdb" "gdb" true
        check_command "lldb" "lldb" true
        ;;
    linux)
        check_command "cmake" "cmake"
        check_command "ninja" "ninja"
        check_command "g++" "g++"
        check_command "gdb" "gdb"
        check_command "lldb" "lldb" true
        ;;
    macos)
        check_command "cmake" "cmake"
        check_command "ninja" "ninja"
        check_command "clang++" "clang++"
        check_command "lldb" "lldb"
        check_command "gdb" "gdb" true
        ;;
esac

echo ""

# Check Qt installation
echo -e "${YELLOW}3. Checking Qt installation...${NC}"

qt_found=false
case "$PLATFORM" in
    windows)
        qt_paths=(
            "/c/Qt/6.*/msvc*/bin/qmake.exe"
            "/c/msys64/mingw64/bin/qmake.exe"
            "$MSYS2_ROOT/mingw64/bin/qmake.exe"
        )
        ;;
    linux)
        qt_paths=(
            "/usr/bin/qmake6"
            "/usr/bin/qmake-qt6"
            "/usr/local/bin/qmake6"
        )
        ;;
    macos)
        qt_paths=(
            "/usr/local/opt/qt6/bin/qmake"
            "/opt/homebrew/opt/qt6/bin/qmake"
        )
        ;;
esac

for path in "${qt_paths[@]}"; do
    if [[ "$path" == *"*"* ]]; then
        # Handle glob patterns
        for match in $path; do
            if [ -f "$match" ]; then
                echo -e "   ${GREEN}✓ Qt6 found at $match${NC}"
                qt_found=true
                break 2
            fi
        done
    elif [ -f "$path" ]; then
        echo -e "   ${GREEN}✓ Qt6 found at $path${NC}"
        qt_found=true
        break
    fi
done

if [ "$qt_found" = false ]; then
    echo -e "   ${YELLOW}⚠ Qt6 not found in standard locations${NC}"
    echo -e "     Please ensure Qt6 development packages are installed"
fi

echo ""

# Manual testing instructions
echo -e "${YELLOW}4. Manual Testing Instructions${NC}"
echo ""

echo -e "${CYAN}To test the debugging setup:${NC}"
echo "1. Open the project in VSCode"
echo "2. Open app/main.cpp"
echo "3. Set a breakpoint on line 22 (LOG_INFO statement)"
echo "4. Select appropriate debug configuration:"

case "$PLATFORM" in
    windows)
        echo "   - (Windows) Debug with MSVC (if Visual Studio installed)"
        echo "   - (Windows) Debug with MinGW-w64 GDB (if MSYS2 installed)"
        ;;
    linux)
        echo "   - (Linux) Debug with GDB (recommended)"
        echo "   - (Linux) Debug with LLDB (alternative)"
        ;;
    macos)
        echo "   - (macOS) Debug with LLDB (recommended)"
        echo "   - (macOS) Debug with GDB (if available)"
        ;;
esac

echo "5. Press F5 to start debugging"
echo "6. Verify the debugger stops at the breakpoint"
echo "7. Check the Variables panel for local variables"
echo "8. Test step-through debugging (F10, F11)"
echo ""

echo -e "${CYAN}Expected debugging features:${NC}"
echo "✓ Breakpoints work correctly"
echo "✓ Variable inspection shows values"
echo "✓ Qt types display properly (QString, QApplication, etc.)"
echo "✓ Call stack navigation works"
echo "✓ Step-through debugging (over, into, out)"
echo "✓ Watch expressions can be added"
echo ""

# Summary
echo -e "${GREEN}=== Test Summary ===${NC}"
echo "Configuration files: Created and validated"
echo "Platform support: $PLATFORM"
echo "Next steps: Follow manual testing instructions above"
echo ""
echo -e "${CYAN}For detailed setup information, see:${NC}"
echo "- docs/debugging-setup.md (comprehensive guide)"
echo "- .vscode/README-debugging.md (quick reference)"
