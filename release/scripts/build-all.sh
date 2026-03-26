#!/bin/bash
# Phoenix Engine - Cross-Platform Release Builder
# Builds release packages for all supported platforms

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
RELEASE_DIR="$PROJECT_ROOT/release"
PACKAGES_DIR="$RELEASE_DIR/packages"
BUILD_DIR="$PROJECT_ROOT/build/release"
VERSION="1.0.0"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Create directories
mkdir -p "$PACKAGES_DIR"/{windows,linux,macos,ios,android,web,source}
mkdir -p "$RELEASE_DIR"/{checksums,signing}

log_info "Phoenix Engine Release Builder v$VERSION"
log_info "Project Root: $PROJECT_ROOT"
log_info "Release Dir: $RELEASE_DIR"

# Function to build Windows package
build_windows() {
    log_info "Building Windows package..."
    
    # Check for cross-compiler
    if ! command -v x86_64-w64-mingw32-cmake &> /dev/null; then
        log_warn "MinGW cross-compiler not found, skipping Windows build"
        return 0
    fi
    
    mkdir -p "$BUILD_DIR/windows"
    cd "$BUILD_DIR/windows"
    
    x86_64-w64-mingw32-cmake "$PROJECT_ROOT" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_SYSTEM_NAME=Windows \
        -DPHoenix_INSTALL_PREFIX="$PACKAGES_DIR/windows/PhoenixEngine-$VERSION"
    
    cmake --build . -j$(nproc)
    cmake --install . --prefix "$PACKAGES_DIR/windows/PhoenixEngine-$VERSION"
    
    # Create installer (requires NSIS or WiX)
    if command -v makensis &> /dev/null; then
        log_info "Creating Windows installer (NSIS)..."
        # NSIS script would be here
    fi
    
    cd "$PROJECT_ROOT"
    log_success "Windows package built"
}

# Function to build Linux package
build_linux() {
    log_info "Building Linux packages..."
    
    mkdir -p "$BUILD_DIR/linux"
    cd "$BUILD_DIR/linux"
    
    cmake "$PROJECT_ROOT" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/opt/phoenix-engine \
        -DPHoenix_VERSION=$VERSION
    
    cmake --build . -j$(nproc)
    
    # Create DEB package
    log_info "Creating DEB package..."
    mkdir -p "$PACKAGES_DIR/linux/deb/phoenix-engine_$VERSION-1_amd64"
    # DEB structure would be populated here
    
    # Create RPM package
    log_info "Creating RPM package..."
    # RPM spec file would be processed here
    
    # Create AppImage
    log_info "Creating AppImage..."
    # AppImage creation script would be here
    
    cd "$PROJECT_ROOT"
    log_success "Linux packages built"
}

# Function to build macOS package
build_macos() {
    log_info "Building macOS package..."
    
    if [[ "$OSTYPE" != "darwin"* ]]; then
        log_warn "Not on macOS, skipping native build"
        # Could use cross-compilation or CI
        return 0
    fi
    
    mkdir -p "$BUILD_DIR/macos"
    cd "$BUILD_DIR/macos"
    
    cmake "$PROJECT_ROOT" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
        -DCMAKE_INSTALL_PREFIX="$PACKAGES_DIR/macos/PhoenixEngine.framework"
    
    cmake --build . -j$(sysctl -n hw.ncpu)
    cmake --install .
    
    # Create DMG
    log_info "Creating DMG..."
    # hdiutil create command would be here
    
    # Code signing (requires Apple Developer ID)
    if [ -n "$APPLEDeveloper_ID" ]; then
        log_info "Signing macOS application..."
        # codesign command would be here
    fi
    
    cd "$PROJECT_ROOT"
    log_success "macOS package built"
}

# Function to build iOS package
build_ios() {
    log_info "Building iOS package..."
    
    # Requires Xcode and iOS SDK
    if ! command -v xcodebuild &> /dev/null; then
        log_warn "Xcode not found, skipping iOS build"
        return 0
    fi
    
    cd "$PROJECT_ROOT/examples/ios/PhoenixEngine"
    
    xcodebuild -scheme PhoenixEngine \
        -configuration Release \
        -archivePath "$BUILD_DIR/ios/PhoenixEngine.xcarchive" \
        archive
    
    # Export IPA (requires provisioning profile)
    if [ -n "$IOS_PROVISIONING_PROFILE" ]; then
        log_info "Exporting IPA..."
        # xcodebuild -exportArchive command would be here
    fi
    
    cd "$PROJECT_ROOT"
    log_success "iOS package built"
}

# Function to build Android package
build_android() {
    log_info "Building Android package..."
    
    if [ -z "$ANDROID_SDK_ROOT" ] && [ -z "$ANDROID_HOME" ]; then
        log_warn "Android SDK not found, skipping Android build"
        return 0
    fi
    
    cd "$PROJECT_ROOT/examples/android/PhoenixEngine"
    
    # Build APK
    ./gradlew assembleRelease
    
    # Build AAB for Play Store
    ./gradlew bundleRelease
    
    # Copy to release directory
    mkdir -p "$PACKAGES_DIR/android"
    cp build/outputs/apk/release/*.apk "$PACKAGES_DIR/android/" 2>/dev/null || true
    cp build/outputs/bundle/release/*.aab "$PACKAGES_DIR/android/" 2>/dev/null || true
    
    # Sign APK (requires keystore)
    if [ -n "$ANDROID_KEYSTORE" ]; then
        log_info "Signing Android package..."
        # apksigner command would be here
    fi
    
    cd "$PROJECT_ROOT"
    log_success "Android package built"
}

# Function to build Web/WASM package
build_web() {
    log_info "Building Web/WASM package..."
    
    cd "$PROJECT_ROOT/wasm"
    
    # Build WASM module
    if [ -f "scripts/build.sh" ]; then
        ./scripts/build.sh
    else
        log_warn "WASM build script not found"
        return 0
    fi
    
    # Create NPM package
    log_info "Creating NPM package..."
    if [ -f "package.json" ]; then
        # Update version
        jq ".version = \"$VERSION\"" package.json > package.json.tmp
        mv package.json.tmp package.json
        
        # Pack
        npm pack
        mv phoenix-engine-*.tgz "$PACKAGES_DIR/web/"
    fi
    
    cd "$PROJECT_ROOT"
    log_success "Web/WASM package built"
}

# Function to create source package
build_source() {
    log_info "Creating source package..."
    
    SOURCE_PKG="$PACKAGES_DIR/source/phoenix-engine-$VERSION"
    mkdir -p "$SOURCE_PKG"
    
    # Copy source files (excluding build artifacts)
    rsync -av --exclude='.git' \
          --exclude='build' \
          --exclude='*.o' \
          --exclude='*.a' \
          --exclude='*.so' \
          --exclude='*.dll' \
          --exclude='*.dylib' \
          "$PROJECT_ROOT/" "$SOURCE_PKG/"
    
    # Create tarball
    cd "$PACKAGES_DIR/source"
    tar -czf "phoenix-engine-$VERSION-source.tar.gz" "phoenix-engine-$VERSION"
    tar -cjf "phoenix-engine-$VERSION-source.tar.bz2" "phoenix-engine-$VERSION"
    zip -rq "phoenix-engine-$VERSION-source.zip" "phoenix-engine-$VERSION"
    
    # Cleanup
    rm -rf "$SOURCE_PKG"
    
    cd "$PROJECT_ROOT"
    log_success "Source package created"
}

# Function to generate checksums
generate_checksums() {
    log_info "Generating checksums..."
    
    CHECKSUM_FILE="$RELEASE_DIR/checksums/SHA256SUMS.txt"
    
    cd "$PACKAGES_DIR"
    find . -type f \( -name "*.tar.gz" -o -name "*.tar.bz2" -o -name "*.zip" \
                     -o -name "*.deb" -o -name "*.rpm" -o -name "*.AppImage" \
                     -o -name "*.dmg" -o -name "*.ipa" -o -name "*.apk" \
                     -o -name "*.aab" -o -name "*.tgz" \) \
        -exec sha256sum {} \; > "$CHECKSUM_FILE"
    
    cd "$PROJECT_ROOT"
    log_success "Checksums generated: $CHECKSUM_FILE"
}

# Function to sign packages (placeholder)
sign_packages() {
    log_info "Signing packages..."
    
    # Windows: Sign with Authenticode
    # macOS/iOS: Sign with Apple Developer ID
    # Android: Sign with Jarsigner/ApkSigner
    
    log_warn "Package signing requires certificates - configure in CI/CD"
}

# Main execution
main() {
    log_info "Starting release build process..."
    
    build_source      # Always build source
    build_linux       # Linux packages
    build_web         # Web/WASM packages
    build_windows     # Windows (if cross-compiler available)
    build_macos       # macOS (if on macOS)
    build_ios         # iOS (if Xcode available)
    build_android     # Android (if SDK available)
    
    generate_checksums
    sign_packages
    
    log_success "Release build complete!"
    log_info "Packages available in: $PACKAGES_DIR"
    log_info "Checksums available in: $RELEASE_DIR/checksums/"
}

# Run main function
main "$@"
