#!/bin/bash
# Create DEB package for Phoenix Engine

set -e

VERSION="${1:-1.0.0}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build/release"
DEB_DIR="$PROJECT_ROOT/release/packages/linux/deb"
DEB_NAME="phoenix-engine_${VERSION}-1_amd64"

echo "Creating DEB package for Phoenix Engine v$VERSION"

# Create DEB structure
mkdir -p "$DEB_DIR/$DEB_NAME"/{DEBIAN,usr/{bin,lib,share/{doc,icons}}}

# Copy binaries
cp "$BUILD_DIR/bin/"* "$DEB_DIR/$DEB_NAME/usr/bin/" 2>/dev/null || true
cp "$BUILD_DIR/lib/"*.so* "$DEB_DIR/$DEB_NAME/usr/lib/" 2>/dev/null || true

# Create control file
cat > "$DEB_DIR/$DEB_NAME/DEBIAN/control" << EOF
Package: phoenix-engine
Version: $VERSION-1
Section: graphics
Priority: optional
Architecture: amd64
Depends: libvulkan1, libgl1, libx11-6, libxcursor1, libxinerama1, libxrandr2
Maintainer: Phoenix Engine Team <phoenix-engine@example.com>
Description: Cross-platform 3D Rendering Engine
 Phoenix Engine is a high-performance, security-first 3D rendering engine
 supporting Windows, Linux, macOS, Android, iOS, and Web platforms.
 .
 Features:
  - Multi-API rendering (Vulkan, OpenGL, etc.)
  - PBR materials and lighting
  - Scene management system
  - Animation support
  - Rust-based security core
Homepage: https://github.com/phoenix-engine/phoenix-engine
License: MIT
EOF

# Create postinst script
cat > "$DEB_DIR/$DEB_NAME/DEBIAN/postinst" << 'EOF'
#!/bin/bash
ldconfig
echo "Phoenix Engine installed successfully!"
echo "Run 'phoenix-engine-demo' to start the demo."
EOF

chmod +x "$DEB_DIR/$DEB_NAME/DEBIAN/postinst"

# Copy documentation
cp "$PROJECT_ROOT/README.md" "$DEB_DIR/$DEB_NAME/usr/share/doc/phoenix-engine/"
cp "$PROJECT_ROOT/CHANGELOG.md" "$DEB_DIR/$DEB_NAME/usr/share/doc/phoenix-engine/"
cp "$PROJECT_ROOT/LICENSE" "$DEB_DIR/$DEB_NAME/usr/share/doc/phoenix-engine/" 2>/dev/null || true

# Build the package
cd "$DEB_DIR"
dpkg-deb --build "$DEB_NAME"

echo "DEB package created: $DEB_DIR/$DEB_NAME.deb"
