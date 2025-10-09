#!/bin/bash
# Local packaging script for SAST Readium
# Supports creating .deb, .rpm, AppImage, and .dmg packages

set -e

# Script directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
PACKAGE_DIR="$PROJECT_ROOT/packaging"

# Default values
APP_NAME="sast-readium"
APP_DISPLAY_NAME="SAST Readium"
VERSION="0.1.0"
BUILD_TYPE="Release"
PACKAGE_TYPES=""
CLEAN_PACKAGING=false

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Show usage information
show_usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Create platform-specific packages for SAST Readium

OPTIONS:
    -t, --type TYPE         Package type: deb, rpm, appimage, dmg, all (default: auto-detect)
    -v, --version VERSION   Version string (default: $VERSION)
    -b, --build-type TYPE   Build type: Debug or Release (default: $BUILD_TYPE)
    -c, --clean             Clean packaging directory before building
    -h, --help              Show this help message

PACKAGE TYPES:
    deb                     Debian/Ubuntu package (.deb)
    rpm                     Red Hat/Fedora package (.rpm)
    appimage                Universal Linux package (AppImage)
    dmg                     macOS disk image (.dmg)
    all                     Build all supported packages for current platform

EXAMPLES:
    $0                      # Auto-detect platform and create appropriate package
    $0 -t deb               # Create .deb package only
    $0 -t all -v 1.0.0      # Create all packages with version 1.0.0
    $0 -t appimage -c       # Clean build AppImage

REQUIREMENTS:
    Linux (deb):    dpkg-dev, fakeroot
    Linux (rpm):    rpm-build, rpmbuild
    Linux (appimage): appimagetool
    macOS (dmg):    create-dmg (brew install create-dmg)

EOF
}

# Parse command line arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -t|--type)
                PACKAGE_TYPES="$2"
                shift 2
                ;;
            -v|--version)
                VERSION="$2"
                shift 2
                ;;
            -b|--build-type)
                BUILD_TYPE="$2"
                shift 2
                ;;
            -c|--clean)
                CLEAN_PACKAGING=true
                shift
                ;;
            -h|--help)
                show_usage
                exit 0
                ;;
            *)
                print_error "Unknown option: $1"
                show_usage
                exit 1
                ;;
        esac
    done
}

# Detect platform and set default package types
detect_platform() {
    if [[ -z "$PACKAGE_TYPES" ]]; then
        case "$(uname -s)" in
            Linux*)
                PACKAGE_TYPES="deb"
                if command -v rpm &> /dev/null; then
                    PACKAGE_TYPES="$PACKAGE_TYPES rpm"
                fi
                if command -v appimagetool &> /dev/null; then
                    PACKAGE_TYPES="$PACKAGE_TYPES appimage"
                fi
                ;;
            Darwin*)
                PACKAGE_TYPES="dmg"
                ;;
            *)
                print_error "Unsupported platform: $(uname -s)"
                exit 1
                ;;
        esac
    elif [[ "$PACKAGE_TYPES" == "all" ]]; then
        case "$(uname -s)" in
            Linux*)
                PACKAGE_TYPES="deb rpm appimage"
                ;;
            Darwin*)
                PACKAGE_TYPES="dmg"
                ;;
        esac
    fi

    print_status "Package types: $PACKAGE_TYPES"
}

# Check if build exists
check_build() {
    local build_path="$BUILD_DIR/$BUILD_TYPE"
    local app_path="$build_path/app/app"

    if [[ ! -f "$app_path" ]]; then
        print_error "Build not found at $app_path"
        print_error "Please build the project first:"
        print_error "  cmake --preset=$BUILD_TYPE-Unix"
        print_error "  cmake --build --preset=$BUILD_TYPE-Unix"
        exit 1
    fi

    print_success "Found build at $app_path"
}

# Clean packaging directory
clean_packaging() {
    if [[ "$CLEAN_PACKAGING" == "true" ]]; then
        print_status "Cleaning packaging directory..."
        rm -rf "$PACKAGE_DIR"
    fi
    mkdir -p "$PACKAGE_DIR"
}

# Create .deb package
create_deb() {
    print_status "Creating .deb package..."

    local deb_dir="$PACKAGE_DIR/deb"
    mkdir -p "$deb_dir/DEBIAN"
    mkdir -p "$deb_dir/usr/bin"
    mkdir -p "$deb_dir/usr/share/applications"
    mkdir -p "$deb_dir/usr/share/pixmaps"

    # Copy binary
    cp "$BUILD_DIR/$BUILD_TYPE/app/app" "$deb_dir/usr/bin/$APP_NAME"

    # Create desktop file
    cat > "$deb_dir/usr/share/applications/$APP_NAME.desktop" << EOF
[Desktop Entry]
Name=$APP_DISPLAY_NAME
Comment=Qt6-based PDF reader
Exec=$APP_NAME
Icon=$APP_NAME
Terminal=false
Type=Application
Categories=Office;Viewer;
MimeType=application/pdf;
EOF

    # Create control file
    cat > "$deb_dir/DEBIAN/control" << EOF
Package: $APP_NAME
Version: $VERSION
Section: utils
Priority: optional
Architecture: amd64
Depends: libqt6core6, libqt6gui6, libqt6widgets6, libqt6svg6, libpoppler-qt6-3
Maintainer: SAST Team
Description: Qt6-based PDF reader application
 A modern PDF reader built with Qt6 and Poppler.
EOF

    # Build .deb
    local deb_file="$PROJECT_ROOT/${APP_NAME}_${VERSION}_amd64.deb"
    dpkg-deb --build "$deb_dir" "$deb_file"

    print_success "Created $deb_file"
}

# Create .rpm package
create_rpm() {
    print_status "Creating .rpm package..."

    local rpm_dir="$PACKAGE_DIR/rpm"
    mkdir -p "$rpm_dir"/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

    # Create RPM spec file
    cat > "$rpm_dir/SPECS/$APP_NAME.spec" << EOF
Name: $APP_NAME
Version: $VERSION
Release: 1%{?dist}
Summary: Qt6-based PDF reader
License: MIT
URL: https://github.com/SAST-Readium/sast-readium

%description
A modern PDF reader built with Qt6 and Poppler.

%install
mkdir -p %{buildroot}/usr/bin
mkdir -p %{buildroot}/usr/share/applications
cp $BUILD_DIR/$BUILD_TYPE/app/app %{buildroot}/usr/bin/$APP_NAME
cp $PACKAGE_DIR/deb/usr/share/applications/$APP_NAME.desktop %{buildroot}/usr/share/applications/

%files
/usr/bin/$APP_NAME
/usr/share/applications/$APP_NAME.desktop

%changelog
* $(date +'%a %b %d %Y') SAST Team - $VERSION-1
- Release $VERSION
EOF

    # Build RPM
    rpmbuild --define "_topdir $rpm_dir" -bb "$rpm_dir/SPECS/$APP_NAME.spec"
    cp "$rpm_dir/RPMS/x86_64/$APP_NAME-"*.rpm "$PROJECT_ROOT/"

    print_success "Created RPM package"
}

# Create AppImage
create_appimage() {
    print_status "Creating AppImage..."

    if ! command -v appimagetool &> /dev/null; then
        print_error "appimagetool not found. Please install it first."
        return 1
    fi

    local appimage_dir="$PACKAGE_DIR/appimage"
    local appdir="$appimage_dir/$APP_NAME.AppDir"

    mkdir -p "$appdir/usr/bin"
    mkdir -p "$appdir/usr/share/applications"
    mkdir -p "$appdir/usr/share/icons/hicolor/256x256/apps"

    # Copy files
    cp "$BUILD_DIR/$BUILD_TYPE/app/app" "$appdir/usr/bin/$APP_NAME"
    cp "$PACKAGE_DIR/deb/usr/share/applications/$APP_NAME.desktop" "$appdir/"

    # Create AppRun
    cat > "$appdir/AppRun" << EOF
#!/bin/bash
HERE="\$(dirname "\$(readlink -f "\${0}")")"
exec "\${HERE}/usr/bin/$APP_NAME" "\$@"
EOF
    chmod +x "$appdir/AppRun"

    # Build AppImage
    cd "$appimage_dir"
    appimagetool "$APP_NAME.AppDir" "$PROJECT_ROOT/$APP_NAME-$VERSION-x86_64.AppImage"

    print_success "Created AppImage"
}

# Create .dmg (macOS)
create_dmg() {
    print_status "Creating .dmg package..."

    if ! command -v create-dmg &> /dev/null; then
        print_error "create-dmg not found. Please install it with: brew install create-dmg"
        return 1
    fi

    local dmg_dir="$PACKAGE_DIR/dmg"
    local app_bundle="$dmg_dir/$APP_DISPLAY_NAME.app"

    mkdir -p "$app_bundle/Contents/MacOS"
    mkdir -p "$app_bundle/Contents/Resources"

    # Copy binary
    cp "$BUILD_DIR/$BUILD_TYPE/app/app" "$app_bundle/Contents/MacOS/$APP_NAME"
    chmod +x "$app_bundle/Contents/MacOS/$APP_NAME"

    # Create Info.plist
    cat > "$app_bundle/Contents/Info.plist" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>$APP_NAME</string>
    <key>CFBundleIdentifier</key>
    <string>com.sast.readium</string>
    <key>CFBundleName</key>
    <string>$APP_DISPLAY_NAME</string>
    <key>CFBundleVersion</key>
    <string>$VERSION</string>
    <key>CFBundleShortVersionString</key>
    <string>$VERSION</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.15</string>
</dict>
</plist>
EOF

    # Create .dmg
    create-dmg \
        --volname "$APP_DISPLAY_NAME $VERSION" \
        --window-pos 200 120 \
        --window-size 600 300 \
        --icon-size 100 \
        --icon "$APP_DISPLAY_NAME.app" 175 120 \
        --hide-extension "$APP_DISPLAY_NAME.app" \
        --app-drop-link 425 120 \
        "$PROJECT_ROOT/$APP_NAME-$VERSION-universal.dmg" \
        "$dmg_dir/" || true

    print_success "Created .dmg package"
}

# Main function
main() {
    print_status "SAST Readium Packaging Script"
    print_status "=============================="

    parse_args "$@"
    detect_platform
    check_build
    clean_packaging

    # Create packages based on requested types
    for package_type in $PACKAGE_TYPES; do
        case $package_type in
            deb)
                create_deb
                ;;
            rpm)
                create_rpm
                ;;
            appimage)
                create_appimage
                ;;
            dmg)
                create_dmg
                ;;
            *)
                print_warning "Unknown package type: $package_type"
                ;;
        esac
    done

    print_success "Packaging completed successfully!"
    print_status "Generated packages:"
    ls -la "$PROJECT_ROOT"/*.{deb,rpm,AppImage,dmg} 2>/dev/null || true
}

# Run main function
main "$@"
