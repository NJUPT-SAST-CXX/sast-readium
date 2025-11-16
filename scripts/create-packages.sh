#!/bin/bash
# create-packages.sh - Cross-platform packaging script for SAST Readium

set -e

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Configuration
BUILD_TYPE="${BUILD_TYPE:-Release}"
BUILD_SYSTEM="${BUILD_SYSTEM:-cmake}"
PACKAGE_DIR="package"

echo -e "${GREEN}=== SAST Readium Packaging Script ===${NC}"
echo "Build Type: $BUILD_TYPE"
echo "Build System: $BUILD_SYSTEM"
echo ""

# Detect platform
if [[ $OSTYPE == "linux-gnu"* ]]; then
    PLATFORM="linux"
elif [[ $OSTYPE == "darwin"* ]]; then
    PLATFORM="macos"
elif [[ $OSTYPE == "msys" || $OSTYPE == "cygwin" ]]; then
    PLATFORM="windows"
else
    echo -e "${RED}Unsupported platform: $OSTYPE${NC}"
    exit 1
fi

echo "Platform: $PLATFORM"
echo ""

# Create package directory
mkdir -p "$PACKAGE_DIR"

if [ "$BUILD_SYSTEM" == "cmake" ]; then
    echo -e "${GREEN}Using CMake build system${NC}"

    # Configure
    cmake -B build -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DENABLE_PACKAGING=ON \
        -DPACKAGE_PORTABLE=ON \
        -DPACKAGE_INSTALLER=ON \
        -DSAST_QUIET=ON \
        -DSAST_ENABLE_HARDENING=ON \
        $([ "$BUILD_TYPE" == "Release" ] && echo "-DSAST_ENABLE_LTO=ON")

    # Build
    cmake --build build --config $BUILD_TYPE -j$(nproc 2>/dev/null || echo 4)

    # Package
    cd build
    cpack -C $BUILD_TYPE
    cd ..

    echo -e "${GREEN}CMake packages created in build/package/${NC}"

elif [ "$BUILD_SYSTEM" == "xmake" ]; then
    echo -e "${GREEN}Using XMake build system${NC}"

    # Configure
    xmake f -m $(echo "$BUILD_TYPE" | tr '[:upper:]' '[:lower:]') \
        --enable_packaging=y \
        --quiet=y \
        --enable_hardening=y \
        $([ "$BUILD_TYPE" == "Release" ] && echo "--enable_lto=y")

    # Build and package
    xmake

    echo -e "${GREEN}XMake packages created in package/${NC}"
else
    echo -e "${RED}Unknown build system: $BUILD_SYSTEM${NC}"
    exit 1
fi

# List created packages
echo ""
echo -e "${GREEN}=== Created Packages ===${NC}"
find "$PACKAGE_DIR" -type f \( -name "*.zip" -o -name "*.tar.gz" -o -name "*.deb" -o -name "*.rpm" -o -name "*.dmg" -o -name "*.AppImage" -o -name "*.exe" -o -name "*.msi" \) -exec ls -lh {} \;

echo ""
echo -e "${GREEN}=== Checksums ===${NC}"
find "$PACKAGE_DIR" -name "*.sha256" -exec cat {} \;

echo ""
echo -e "${GREEN}Packaging completed successfully!${NC}"
