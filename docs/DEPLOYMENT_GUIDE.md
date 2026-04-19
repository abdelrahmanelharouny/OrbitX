# 🚀 Deployment & Packaging Guide

This guide walks you through packaging the Anti-Gravity Solar System for distribution on Windows, macOS, and Linux.

---

## 📦 Overview

### Distribution Formats

| Platform | Format | Description |
|----------|--------|-------------|
| **Windows** | `.zip` with `.exe` | Portable executable |
| **Windows** | `.msi` installer | Professional installer (optional) |
| **macOS** | `.dmg` | Disk image with `.app` bundle |
| **Linux** | `.tar.gz` | Compressed archive |
| **Linux** | AppImage | Universal package (optional) |

---

## 🛠️ Build Configuration

### CMake Build Types

```bash
# Debug build (development)
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug

# Release build (distribution)
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
```

### Compiler Optimizations

For maximum performance in release builds:

#### GCC/Clang
```bash
cmake -S . -B build-release \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS="-O3 -march=native -flto"
```

#### MSVC (Windows)
```bash
cmake -S . -B build-release \
  -DCMAKE_CXX_FLAGS_RELEASE="/O2 /GL /LTCG"
```

---

## 🪟 Windows Deployment

### Step 1: Build Release Executable

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Step 2: Gather Dependencies

The application requires these runtime files:

```
build/Release/
├── AntiGravitySolarSystem.exe
├── glfw3.dll              # Window management
├── OpenGL32.dll           # System library (usually present)
└── assets/                # Application data
    ├── data/
    └── shaders/
```

### Step 3: Create Distribution Package

Create a folder structure for distribution:

```
AntiGravitySolarSystem-v1.0-Windows/
├── AntiGravitySolarSystem.exe
├── README.txt
├── LICENSE.txt
├── assets/
│   ├── data/
│   │   ├── solar_system.json
│   │   └── presets/
│   └── shaders/
│       ├── planet.vert
│       ├── planet.frag
│       └── ...
└── dependencies/
    └── (any additional DLLs)
```

**PowerShell script to create package:**

```powershell
$version = "1.0"
$distFolder = "AntiGravitySolarSystem-v${version}-Windows"

New-Item -ItemType Directory -Force -Path $distFolder
New-Item -ItemType Directory -Force -Path "$distFolder\assets\data"
New-Item -ItemType Directory -Force -Path "$distFolder\assets\shaders"

Copy-Item "build\Release\AntiGravitySolarSystem.exe" -Destination $distFolder
Copy-Item "build\_deps\glfw-src\src\Release\glfw3.dll" -Destination $distFolder
Copy-Item "assets\data\*" -Destination "$distFolder\assets\data" -Recurse
Copy-Item "assets\shaders\*" -Destination "$distFolder\assets\shaders" -Recurse
Copy-Item "README.md" -Destination "$distFolder\README.txt"
Copy-Item "LICENSE" -Destination "$distFolder\LICENSE.txt"

Compress-Archive -Path $distFolder -DestinationPath "${distFolder}.zip" -Force
```

### Step 4: Test on Clean System

⚠️ **Critical**: Test your package on a Windows VM or clean system without development tools installed.

```powershell
# On clean system
.\AntiGravitySolarSystem.exe
```

If it fails with missing DLL errors, you need to include those DLLs.

---

## 🍎 macOS Deployment

### Step 1: Build Release

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Step 2: Create App Bundle

Create `AntiGravitySolarSystem.app` structure:

```
AntiGravitySolarSystem.app/
├── Contents/
│   ├── Info.plist
│   ├── MacOS/
│   │   └── AntiGravitySolarSystem
│   ├── Resources/
│   │   ├── assets/
│   │   └── icon.icns
│   └── Frameworks/
│       └── (embedded frameworks)
```

**Info.plist template:**

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" 
  "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>AntiGravitySolarSystem</string>
    <key>CFBundleIdentifier</key>
    <string>com.yourname.antigravitysolarsystem</string>
    <key>CFBundleName</key>
    <string>Anti-Gravity Solar System</string>
    <key>CFBundleVersion</key>
    <string>1.0.0</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0.0</string>
    <key>NSHighResolutionCapable</key>
    <true/>
</dict>
</plist>
```

**Create app bundle script:**

```bash
#!/bin/bash

APP_NAME="AntiGravitySolarSystem"
APP_BUNDLE="${APP_NAME}.app"
VERSION="1.0"

# Create bundle structure
mkdir -p "${APP_BUNDLE}/Contents/MacOS"
mkdir -p "${APP_BUNDLE}/Contents/Resources/assets"

# Copy executable
cp "build/${APP_NAME}" "${APP_BUNDLE}/Contents/MacOS/"

# Copy assets
cp -r "assets/data" "${APP_BUNDLE}/Contents/Resources/assets/"
cp -r "assets/shaders" "${APP_BUNDLE}/Contents/Resources/assets/"

# Copy Info.plist
cp "Info.plist" "${APP_BUNDLE}/Contents/"

# Make executable
chmod +x "${APP_BUNDLE}/Contents/MacOS/${APP_NAME}"

# Create DMG
hdiutil create -volname "${APP_NAME}-v${VERSION}" \
  -srcfolder "${APP_BUNDLE}" \
  -ov -format UDZO "${APP_NAME}-v${VERSION}.dmg"
```

### Step 3: Code Signing (Optional but Recommended)

```bash
# Sign the app (requires Apple Developer account)
codesign --deep --force --verify --verbose \
  --sign "Developer ID Application: Your Name" \
  "${APP_BUNDLE}"

# Notarize (for distribution outside App Store)
xcrun notarytool submit "${APP_NAME}-v${VERSION}.dmg" \
  --apple-id "your@email.com" \
  --password "app-specific-password" \
  --team-id "YOUR_TEAM_ID" \
  --wait
```

---

## 🐧 Linux Deployment

### Step 1: Build Release

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Step 2: Create Archive

```bash
#!/bin/bash

APP_NAME="AntiGravitySolarSystem"
VERSION="1.0"
DIST_FOLDER="${APP_NAME}-v${VERSION}-Linux"

mkdir -p "${DIST_FOLDER}"
mkdir -p "${DIST_FOLDER}/assets/data"
mkdir -p "${DIST_FOLDER}/assets/shaders"

cp "build/${APP_NAME}" "${DIST_FOLDER}/"
cp -r "assets/data/"* "${DIST_FOLDER}/assets/data/"
cp -r "assets/shaders/"* "${DIST_FOLDER}/assets/shaders/"
cp "README.md" "${DIST_FOLDER}/"
cp "LICENSE" "${DIST_FOLDER}/"

tar -czf "${DIST_FOLDER}.tar.gz" "${DIST_FOLDER}"
```

### Step 3: Create AppImage (Optional)

AppImage provides a universal Linux package.

**Install linuxdeploy:**

```bash
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
chmod +x linuxdeploy-x86_64.AppImage
```

**Create AppDir structure:**

```bash
mkdir -p AppDir/usr/bin
mkdir -p AppDir/usr/share/applications
mkdir -p AppDir/usr/share/icons/hicolor/256x256/apps

cp build/AntiGravitySolarSystem AppDir/usr/bin/
cp -r assets AppDir/usr/bin/

# Create desktop file
cat > AppDir/usr/share/applications/antigravitysolarsystem.desktop << EOF
[Desktop Entry]
Type=Application
Name=Anti-Gravity Solar System
Exec=AntiGravitySolarSystem
Icon=antigravitysolarsystem
Categories=Game;Simulation;
EOF

# Copy icon
cp icons/icon_256.png AppDir/usr/share/icons/hicolor/256x256/apps/antigravitysolarsystem.png
```

**Build AppImage:**

```bash
./linuxdeploy-x86_64.AppImage --appdir AppDir \
  --output appimage \
  --desktop-file=AppDir/usr/share/applications/antigravitysolarsystem.desktop
```

---

## 📋 Pre-Distribution Checklist

Before releasing your package:

### Code Quality
- [ ] No debug output in console
- [ ] All assertions disabled
- [ ] Error messages are user-friendly
- [ ] No memory leaks (verified with Valgrind/Visual Leak Detector)

### Functionality
- [ ] Application starts without errors
- [ ] All presets load correctly
- [ ] UI is responsive
- [ ] Controls work as documented
- [ ] Performance meets targets (60 FPS)

### Assets
- [ ] All required files included
- [ ] Shaders compile correctly
- [ ] JSON files are valid
- [ ] Icons display properly

### Documentation
- [ ] README included
- [ ] License file included
- [ ] Controls documented
- [ ] System requirements listed

### Testing
- [ ] Tested on clean Windows VM
- [ ] Tested on clean macOS (if applicable)
- [ ] Tested on multiple Linux distributions (if applicable)
- [ ] Verified no admin privileges required

---

## 🔧 CPack Integration (Automated Packaging)

Add this to your `CMakeLists.txt` for automated packaging:

```cmake
# At the end of CMakeLists.txt

set(CPACK_PACKAGE_NAME "AntiGravitySolarSystem")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VENDOR "Your Name")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "N-Body Gravitational Simulation")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/yourusername/anti-gravity-solar-system")

# General settings
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

# Include assets
install(DIRECTORY assets/ DESTINATION assets)

# Windows-specific
if(WIN32)
    set(CPACK_GENERATOR "ZIP")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "AntiGravitySolarSystem")
endif()

# macOS-specific
if(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_BUNDLE_NAME "AntiGravitySolarSystem")
endif()

# Linux-specific
if(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)
```

**Build with CPack:**

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cd build
cpack
```

---

## 📊 File Size Optimization

### Reduce Executable Size

```bash
# Strip symbols (Linux/macOS)
strip build/AntiGravitySolarSystem

# Windows: Use Release configuration (symbols already separate)
```

### Compress Assets

```bash
# Optimize JSON files (remove whitespace)
python -m json.tool --compact solar_system.json > solar_system_min.json
```

### Typical Package Sizes

| Component | Size |
|-----------|------|
| Executable | 500 KB - 2 MB |
| Dependencies | 1-5 MB |
| Assets | 500 KB - 2 MB |
| **Total** | **2-10 MB** |

---

## 🌐 Distribution Platforms

Consider distributing on:

1. **GitHub Releases** (Recommended)
   - Free for open source
   - Automatic binary hosting
   - Version tracking

2. **Itch.io**
   - Good for interactive projects
   - Built-in download analytics

3. **Personal Website**
   - Full control
   - Hosting costs apply

---

## 📈 Post-Release

After publishing:

- [ ] Monitor GitHub issues for bug reports
- [ ] Collect user feedback
- [ ] Track download statistics
- [ ] Update portfolio with release announcement
- [ ] Share on social media and LinkedIn

---

**Distribution complete!** Your application is now ready for users worldwide. Remember to keep backups of all release packages and maintain clear version history.
