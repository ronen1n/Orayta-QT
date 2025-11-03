# Orayta Android

A modern Android application for studying Jewish texts, built with Qt6.

## 📱 Android Features
- Modern mobile UI with gesture and swipe navigation
- Download management with stop functionality
- RTL (Right-to-Left) text display support
- Customizable interface: text-only or icon-only buttons
- Auto-detection of system dark/light theme
- Advanced text search with improved results
- Extensive library of Jewish texts, on-demand downloads
- Multi-language support: Hebrew, French, English
- Bookmark and annotation system

## 🔧 Technical Improvements
- Qt5 → Qt6 migration with modern CMake build system
- Android 16KB page size support for latest Android versions
- Edge-to-edge display with Material Design
- Runtime permissions for modern Android security
- All compiler warnings fixed and code optimized
- Suppressed legacy C warnings in third-party libraries

## Building

### Prerequisites
- Qt6 with Android support
- Android SDK (API 34+) and NDK
- CMake 3.16+

## 🏗️ Build System
- Separate build scripts for ARM64 and ARMv7 architectures
- Regular (small APK) or with-books (offline) build options
- CMake configuration with Qt6 and warning suppression
- PowerShell build scripts for Windows
- Automated translation compilation
- Android environment setup scripts

### Quick Build

```bash
# ARM64 (recommended for modern devices)
./build-android-arm64.ps1

# ARMv7 (older devices compatibility)  
./build-android-armv7.ps1

# With all books included (large offline APK)
./build-android-with-books.ps1
```

### Manual Build
1. Set up your Android SDK path in \android-orayta/local.properties\
2. Configure Qt6 Android environment
3. Run the build script for your target architecture

## Project Structure

- `Mobile/` - Android-specific UI and functionality
- `OraytaBase/` - Core library (text processing, search, etc.)
- `android-orayta/` - Android project configuration
- `openssl-android/` - OpenSSL 3.x libraries for HTTPS/TLS support
- `build-*.ps1` - Build scripts for different architectures

## 🚀 Key Improvements from Original

### Core Fixes & Modernization
- Qt5 → Qt6 migration with CMake build system replacing qmake
- All compiler warnings fixed (41+ files), including nodiscard, tautological comparison, and switch statement warnings
- Suppressed legacy C warnings in QuaZip
- Qt6 compatibility layer for text encoding

### Android-Specific Enhancements
- Android 16KB page size support for latest requirements
- Edge-to-edge Material Design UI
- Runtime permissions and enhanced JNI integration
- Improved gesture and swipe handling
- Download management with stop and progress tracking
- OpenSSL 3.x for HTTPS/TLS

### UI/UX Improvements
- Redesigned mobile UI for Android
- Auto-detection of system dark/light themes
- RTL support for Hebrew
- Customizable interface: text-only or icon-only buttons
- Improved search functionality and result display

### Build System & Development
- Multiple build variants: regular (small) and with-books (offline)
- ARM64 and ARMv7 support
- Automated translation compilation (Hebrew, French)
- Comprehensive build and deployment scripts

## Contributing

This is a focused Android-only version of the Orayta project, optimized for modern Android development.

### Code Quality
- All compiler warnings addressed across 41+ modified files
- Modern C++ practices with Qt6 APIs
- Comprehensive error handling and memory management
- Production-ready with debug code removed

### Security & Performance
- No hardcoded credentials or sensitive paths
- Proper Android permissions model
- Release builds optimized with Proguard
- Symbol stripping and code obfuscation enabled

## License

GNU General Public License v2.0 - see LICENSE file for details.

## 📊 Project Statistics

- **Files Modified**: 41 core files with improvements
- **New Features**: 45+ new files and enhancements
- **Build Variants**: 3 different build configurations
- **Architectures**: ARM64 and ARMv7 support
- **Languages**: Hebrew, French, English with RTL support
- **Compatibility**: Android API 34+ with 16KB page size support

## 🔄 Migration from Original

This project represents a complete modernization of the original Orayta:

### What's Included
- ✅ **Mobile Module**: Complete Android UI and functionality
- ✅ **OraytaBase Library**: Core text processing and search
- ✅ **Android Project**: Full Android configuration and resources
- ✅ **Build System**: Modern CMake with Qt6
- ✅ **Translations**: Hebrew and French language support
- ✅ **Documentation**: Comprehensive build and usage guides

## Acknowledgments

Based on the original Orayta project by Moshe Wagner with significant Android-focused improvements, Qt6 migration, and modern Android development practices.
