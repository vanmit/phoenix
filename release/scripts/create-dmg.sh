#!/bin/bash
# Create DMG for Phoenix Engine (macOS)

set -e

VERSION="${1:-1.0.0}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build/release"
DMG_DIR="$PROJECT_ROOT/release/packages/macos"
APP_NAME="PhoenixEngine"

echo "Creating DMG for $APP_NAME v$VERSION"

# Create application bundle structure
APP_BUNDLE="$DMG_DIR/$APP_NAME.app"
mkdir -p "$APP_BUNDLE"/{Contents/{MacOS,Resources,Frameworks},_CodeSignature}

# Copy binary
cp "$BUILD_DIR/bin/$APP_NAME" "$APP_BUNDLE/Contents/MacOS/" 2>/dev/null || true

# Copy libraries
cp "$BUILD_DIR/lib/"*.dylib "$APP_BUNDLE/Contents/Frameworks/" 2>/dev/null || true

# Create Info.plist
cat > "$APP_BUNDLE/Contents/Info.plist" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleDevelopmentRegion</key>
    <string>en</string>
    <key>CFBundleExecutable</key>
    <string>$APP_NAME</string>
    <key>CFBundleIdentifier</key>
    <string>com.phoenix-engine.engine</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleName</key>
    <string>$APP_NAME</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>$VERSION</string>
    <key>CFBundleVersion</key>
    <string>1</string>
    <key>LSMinimumSystemVersion</key>
    <string>11.0</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>NSPrincipalClass</key>
    <string>NSApplication</string>
</dict>
</plist>
EOF

# Create PkgInfo
echo "APPL????" > "$APP_BUNDLE/Contents/PkgInfo"

# Create simple icon (placeholder)
# In production, this should be a proper .icns file
cat > "$APP_BUNDLE/Contents/Resources/icon.png" << 'EOF'
EOF

# Create DMG
cd "$DMG_DIR"

# Create temporary DMG
hdiutil create -volname "$APP_NAME" \
    -srcfolder "$APP_NAME.app" \
    -ov -format UDRW \
    temp.dmg

# Convert to compressed DMG
hdiutil convert temp.dmg -format UDZO -o "$APP_NAME-$VERSION.dmg"

# Cleanup
rm -f temp.dmg
rm -rf "$APP_BUNDLE"

echo "DMG created: $DMG_DIR/$APP_NAME-$VERSION.dmg"
