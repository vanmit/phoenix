#!/bin/bash
# Create AppImage for Phoenix Engine

set -e

VERSION="${1:-1.0.0}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build/release"
APPIMAGE_DIR="$PROJECT_ROOT/release/packages/linux/appimage"
OUTPUT_DIR="$PROJECT_ROOT/release/packages/linux"

echo "Creating AppImage for Phoenix Engine v$VERSION"

# Create AppDir structure
APPDIR="$APPIMAGE_DIR/PhoenixEngine.AppDir"
mkdir -p "$APPDIR"/{usr/{bin,lib,share/applications,icons/hicolor/256x256/apps},opt}

# Copy application files
cp "$BUILD_DIR/bin/"* "$APPDIR/usr/bin/" 2>/dev/null || true
cp "$BUILD_DIR/lib/"*.so* "$APPDIR/usr/lib/" 2>/dev/null || true

# Create desktop file
cat > "$APPDIR/usr/share/applications/phoenix-engine.desktop" << EOF
[Desktop Entry]
Type=Application
Name=Phoenix Engine
Comment=Cross-platform 3D Rendering Engine
Exec=phoenix-engine-demo
Icon=phoenix-engine
Categories=Graphics;3DGraphics;
Keywords=3d;rendering;engine;graphics;
EOF

# Create AppImage metadata
cat > "$APPDIR/phoenix-engine.desktop" << EOF
[Desktop Entry]
Type=Application
Name=Phoenix Engine
Comment=Cross-platform 3D Rendering Engine
Exec=phoenix-engine-demo
Icon=phoenix-engine
Categories=Graphics;3DGraphics;
EOF

# Create simple icon (placeholder - should be replaced with actual icon)
cat > "$APPDIR/phoenix-engine.svg" << 'EOF'
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 256 256">
  <circle cx="128" cy="128" r="120" fill="#6366f1"/>
  <path d="M128 40 L180 140 L128 220 L76 140 Z" fill="white"/>
</svg>
EOF

cp "$APPDIR/phoenix-engine.svg" "$APPDIR/usr/share/icons/hicolor/256x256/apps/phoenix-engine.svg"

# Create AppRun script
cat > "$APPDIR/AppRun" << 'EOF'
#!/bin/bash
SELF=$(readlink -f "$0")
HERE=${SELF%/*}
export PATH="${HERE}/usr/bin:${PATH}"
export LD_LIBRARY_PATH="${HERE}/usr/lib:${LD_LIBRARY_PATH}"
exec "${HERE}/usr/bin/phoenix-engine-demo" "$@"
EOF

chmod +x "$APPDIR/AppRun"

# Download and use appimagetool
if ! command -v appimagetool &> /dev/null; then
    echo "Downloading appimagetool..."
    wget -q https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage
    chmod +x appimagetool-x86_64.AppImage
    APPIMAGETOOL="./appimagetool-x86_64.AppImage"
else
    APPIMAGETOOL="appimagetool"
fi

# Build AppImage
cd "$APPIMAGE_DIR"
ARCH=x86_64 "$APPIMAGETOOL" "$APPDIR" "$OUTPUT_DIR/PhoenixEngine-$VERSION.AppImage"

echo "AppImage created: $OUTPUT_DIR/PhoenixEngine-$VERSION.AppImage"

# Cleanup
rm -f "$APPIMAGE_DIR/appimagetool-x86_64.AppImage"
