# Phoenix Engine 部署指南

生产环境部署 Phoenix Engine 的完整指南。

## 📋 部署前检查

### 系统要求验证

```bash
# 检查 Vulkan 支持
vulkaninfo | head -20

# 检查 Metal 支持 (macOS)
system_profiler SPDisplaysDataType | grep Metal

# 检查 DX12 支持 (Windows)
dxdiag

# 检查内存
free -h  # Linux
sysctl hw.memsize  # macOS
```

### 依赖检查

```bash
# Linux
ldd ./phoenix-app | grep "not found"

# macOS
otool -L ./phoenix-app | grep "not found"

# Windows
# 使用 Dependencies 工具 (https://github.com/lucasg/Dependencies)
```

## 🐧 Linux 部署

### 系统级安装

```bash
# 1. 安装运行时依赖
sudo apt-get install -y \
    libvulkan1 \
    libglfw3 \
    libassimp5 \
    libpng16-16 \
    libjpeg-turbo8 \
    libfreetype6

# 2. 安装应用
sudo cp bin/phoenix-app /usr/local/bin/
sudo cp -r share/phoenix /usr/local/share/

# 3. 创建桌面快捷方式
sudo cp phoenix-app.desktop /usr/share/applications/

# 4. 验证
phoenix-app --version
```

### AppImage 打包

```bash
# 1. 安装 linuxdeploy
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
chmod +x linuxdeploy-x86_64.AppImage

# 2. 创建 AppDir
mkdir -p AppDir/usr/bin
mkdir -p AppDir/usr/lib
mkdir -p AppDir/usr/share

cp bin/phoenix-app AppDir/usr/bin/
cp -r share/* AppDir/usr/share/

# 3. 创建 AppImage
./linuxdeploy-x86_64.AppImage \
    --appdir AppDir \
    --output appimage \
    --executable AppDir/usr/bin/phoenix-app
```

### Flatpak 打包

```yaml
# phoenix-app.yml
app-id: dev.phoenix.PhoenixApp
runtime: org.freedesktop.Platform
runtime-version: '22.08'
sdk: org.freedesktop.Sdk
command: phoenix-app
modules:
  - name: phoenix-app
    buildsystem: cmake
    sources:
      - type: git
        url: https://github.com/phoenix-engine/phoenix.git
        tag: v1.0.0
```

```bash
# 构建 Flatpak
flatpak-builder build phoenix-app.yml --force-clean
flatpak install build/phoenix-app.flatpak
```

## 🍎 macOS 部署

### 应用程序包

```bash
# 1. 创建 .app 结构
mkdir -p PhoenixApp.app/Contents/MacOS
mkdir -p PhoenixApp.app/Contents/Resources

# 2. 复制文件
cp bin/phoenix-app PhoenixApp.app/Contents/MacOS/
cp -r share/* PhoenixApp.app/Contents/Resources/

# 3. 创建 Info.plist
cat > PhoenixApp.app/Contents/Info.plist << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>phoenix-app</string>
    <key>CFBundleIdentifier</key>
    <string>dev.phoenix.PhoenixApp</string>
    <key>CFBundleVersion</key>
    <string>1.0.0</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
</dict>
</plist>
EOF

# 4. 签名 (可选，用于分发)
codesign --force --deep --sign - PhoenixApp.app
```

### DMG 打包

```bash
# 使用 create-dmg
brew install create-dmg

create-dmg \
    --volname "Phoenix App" \
    --window-pos 200 120 \
    --window-size 600 400 \
    --icon-size 100 \
    --app-drop-link 400 200 \
    "PhoenixApp-1.0.0.dmg" \
    "PhoenixApp.app"
```

## 🪟 Windows 部署

### 安装程序 (NSIS)

```nsis
; phoenix-installer.nsi
!include "MUI2.nsh"

Name "Phoenix App"
OutFile "PhoenixApp-1.0.0-Installer.exe"
InstallDir "$PROGRAMFILES\Phoenix App"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

Section "Install"
    SetOutPath "$INSTDIR"
    File /r "bin\*.*"
    File /r "share\*.*"
    WriteUninstaller "$INSTDIR\Uninstall.exe"
    
    CreateDirectory "$SMPROGRAMS\Phoenix App"
    CreateShortCut "$SMPROGRAMS\Phoenix App\Phoenix App.lnk" "$INSTDIR\phoenix-app.exe"
SectionEnd

Section "Uninstall"
    Delete "$INSTDIR\Uninstall.exe"
    RMDir /r "$INSTDIR"
    Delete "$SMPROGRAMS\Phoenix App\Phoenix App.lnk"
    RMDir "$SMPROGRAMS\Phoenix App"
SectionEnd
```

```bash
# 编译安装程序
makensis phoenix-installer.nsi
```

### MSIX 打包

```xml
<!-- AppxManifest.xml -->
<?xml version="1.0" encoding="utf-8"?>
<Package xmlns="http://schemas.microsoft.com/appx/manifest/foundation/windows10">
  <Identity Name="PhoenixEngine.PhoenixApp" Version="1.0.0.0" Publisher="CN=Phoenix Engine" />
  <Properties>
    <DisplayName>Phoenix App</DisplayName>
    <PublisherDisplayName>Phoenix Engine Team</PublisherDisplayName>
    <Logo>Assets\logo.png</Logo>
  </Properties>
  <Dependencies>
    <TargetDeviceFamily Name="Windows.Desktop" MinVersion="10.0.17763.0" />
  </Dependencies>
  <Applications>
    <Application Id="PhoenixApp" Executable="bin\phoenix-app.exe">
      <VisualElements DisplayName="Phoenix App" Description="3D Rendering App" />
    </Application>
  </Applications>
</Package>
```

## 🤖 Android 部署

### APK 构建

```bash
# 1. 配置签名
keytool -genkey -v -keystore phoenix.keystore -alias phoenix -keyalg RSA -keysize 2048 -validity 10000

# 2. 构建 Debug APK
./gradlew assembleDebug

# 3. 构建 Release APK
./gradlew assembleRelease

# 4. 签名 Release APK
jarsigner -verbose -sigalg SHA1withRSA -digestalg SHA1 \
    -keystore phoenix.keystore \
    app/build/outputs/apk/release/app-release-unsigned.apk \
    phoenix

# 5. 对齐 APK
zipalign -v 4 app-release-unsigned.apk phoenix-app-release.apk
```

### Google Play 发布

```bash
# 1. 构建 AAB (Android App Bundle)
./gradlew bundleRelease

# 2. 上传到 Google Play Console
# https://play.google.com/console
```

## 🍎 iOS 部署

### App Store 发布

```bash
# 1. 配置证书和描述文件
# 在 Apple Developer Portal 创建

# 2. 归档应用
xcodebuild -scheme PhoenixApp \
    -configuration Release \
    -archivePath build/PhoenixApp.xcarchive \
    archive

# 3. 导出 IPA
xcodebuild -exportArchive \
    -archivePath build/PhoenixApp.xcarchive \
    -exportPath build/ipa \
    -exportOptionsPlist ExportOptions.plist

# 4. 上传到 App Store Connect
xcrun altool --upload-app \
    -f build/ipa/PhoenixApp.ipa \
    -t ios \
    -u your.apple.id@example.com \
    -p your-app-specific-password
```

## 🌐 Web 部署

### 静态资源部署

```bash
# 1. 构建 WebAssembly
cd build-wasm
emmake make -j$(nproc)

# 2. 优化
opt -O3 phoenix-app.wasm -o phoenix-app-opt.wasm

# 3. 部署到 CDN
aws s3 cp . s3://your-bucket/phoenix-app/ \
    --recursive \
    --acl public-read \
    --cache-control "max-age=31536000"

# 4. 配置 CloudFront
# 创建 CloudFront 分发，指向 S3 bucket
```

### Service Worker 缓存

```javascript
// sw.js
const CACHE_NAME = 'phoenix-v1';
const ASSETS = [
    '/',
    '/index.html',
    '/phoenix-app.wasm',
    '/phoenix-app.js',
];

self.addEventListener('install', (event) => {
    event.waitUntil(
        caches.open(CACHE_NAME).then((cache) => cache.addAll(ASSETS))
    );
});

self.addEventListener('fetch', (event) => {
    event.respondWith(
        caches.match(event.request).then((response) => {
            return response || fetch(event.request);
        })
    );
});
```

## 🚀 性能优化

### 启动优化

```bash
# 预加载共享库 (Linux)
sudo ldconfig

# 使用 Huge Pages (Linux)
sudo sysctl -w vm.nr_hugepages=1024

# 禁用 ASLR (性能测试)
echo 0 | sudo tee /proc/sys/kernel/randomize_va_space
```

### 运行时优化

```bash
# 设置环境变量
export PHOENIX_VSYNC=0          # 禁用垂直同步
export PHOENIX_MAX_FPS=144      # 最大帧率
export PHOENIX_LOD_BIAS=-1      # LOD 偏置
export PHOENIX_TEXTURE_QUALITY=high  # 纹理质量

# 运行
./phoenix-app --fullscreen --vsync=off
```

## 🔒 安全考虑

### 代码签名

```bash
# macOS 签名
codesign --force --deep --sign "Developer ID" PhoenixApp.app

# Windows 签名 (需要证书)
signtool sign /f certificate.pfx /p password /tr http://timestamp.digicert.com /td sha256 /fd sha256 phoenix-app.exe
```

### 权限配置

```xml
<!-- Android: AndroidManifest.xml -->
<uses-permission android:name="android.permission.INTERNET" />
<uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE" 
    android:maxSdkVersion="28" />
```

## 📊 监控与日志

### 日志配置

```bash
# 生产环境日志
export PHOENIX_LOG_LEVEL=warning
export PHOENIX_LOG_FILE=/var/log/phoenix-app.log
export PHOENIX_LOG_ROTATE=daily

# 开发环境日志
export PHOENIX_LOG_LEVEL=debug
export PHOENIX_LOG_TO_CONSOLE=1
```

### 性能监控

```bash
# 启用性能分析
export PHOENIX_PROFILER=1
export PHOENIX_PROFILER_OUTPUT=trace.json

# 查看性能报告
phoenix-app --profiler-report
```

## 🆘 故障排除

### 常见问题

**应用无法启动**
```bash
# 检查依赖
ldd ./phoenix-app

# 查看日志
tail -f /var/log/phoenix-app.log

# 检查权限
chmod +x ./phoenix-app
```

**性能问题**
```bash
# 检查 GPU 使用
nvidia-smi  # NVIDIA
radeontop   # AMD
intel_gpu_top # Intel

# 检查内存使用
htop
```

**崩溃问题**
```bash
# 启用核心转储 (Linux)
ulimit -c unlimited

# 分析核心转储
gdb ./phoenix-app core
bt
```

---
*最后更新：2026-03-26*
