cmake_minimum_required(VERSION 3.16)

# Project configuration
project(Orayta VERSION 0.96 LANGUAGES CXX C)

# Set C++ standard to C++17 as required for Qt6
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Qt's automatic MOC, UIC, and RCC processing
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Help CMake find Qt6 installation
if(ANDROID)
    # For Android builds, set QT_HOST_PATH as cache variable BEFORE find_package
    if(NOT DEFINED QT_HOST_PATH)
        set(QT_HOST_PATH "C:/Qt/6.10.0/msvc2022_64" CACHE PATH "Qt host path")
    endif()
    
    if(NOT DEFINED CMAKE_PREFIX_PATH)
        set(CMAKE_PREFIX_PATH "C:/Qt/6.10.0/android_arm64_v8a")
    endif()
elseif(WIN32)
    # Desktop builds
    if(NOT DEFINED CMAKE_PREFIX_PATH)
        set(CMAKE_PREFIX_PATH "C:/Qt/6.10.0/msvc2022_64")
    endif()
endif()

# Find Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Gui 
    Widgets 
    Network
    PrintSupport
)

# Platform detection and configuration
if(ANDROID)
    message(STATUS "Configuring for Android platform")
    set(MOBILE ON)
    # Try to find CorePrivate for JNI functionality (optional)
    find_package(Qt6 COMPONENTS CorePrivate)
    
elseif(WIN32)
    message(STATUS "Configuring for Windows platform")
    set(MOBILE OFF)
    
elseif(UNIX AND NOT APPLE)
    message(STATUS "Configuring for Linux platform") 
    set(MOBILE OFF)
    
else()
    message(WARNING "Unsupported platform detected")
    set(MOBILE OFF)
endif()

# Conditional compilation definitions
if(MOBILE)
    add_compile_definitions(MOBILE)
    add_compile_definitions(QTSCROLLER_NO_WEBKIT)
    message(STATUS "Mobile build configuration enabled")
else()
    message(STATUS "Desktop build configuration enabled")
endif()

# Global compile definitions for Qt6 migration
add_compile_definitions(
    QT_DISABLE_DEPRECATED_UP_TO=0x060800  # Qt 6.8 compatibility
    QT_DEPRECATED_WARNINGS
)

# Platform-specific configurations
if(ANDROID)
    add_compile_definitions(ANDROID)
    # Kookita configuration for JNI encryption support
    add_compile_definitions(KOOKITA)
    # Enable 16KB page size support for Android 16+
    add_compile_definitions(ANDROID_16KB_PAGE_SIZE_SUPPORT)
    
    # Strip debug symbols for smaller APK size
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -s")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -s")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s")
    
elseif(WIN32)
    # QuaZip static linking on Windows
    add_compile_definitions(QUAZIP_STATIC)
    # Disable QtWebKit (not available in Qt6)
    add_compile_definitions(NO_WEBKIT)
    
elseif(UNIX AND NOT APPLE)
    # Linux-specific configuration
    add_compile_definitions(NO_WEBKIT)
endif()

# Add subdirectories for source organization
add_subdirectory(OraytaBase)

if(MOBILE)
    add_subdirectory(Mobile)
else()
    # Desktop UI disabled - requires Qt5→Qt6 migration work
    # Needs: QRegExp→QRegularExpression, QtWebKit→QtWebEngine
    # add_subdirectory(Desktop)
    message(STATUS "Desktop UI disabled - core library only")
endif()

# Android-specific CMake properties configuration
if(ANDROID)
    # Set Android package source directory
    set(QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android-orayta")
    
    # Configure Android SDK versions
    set(QT_ANDROID_TARGET_SDK_VERSION 35)  # Android 15 (API level 35)
    set(QT_ANDROID_MIN_SDK_VERSION 28)     # Android 9.0 (API level 28) - Qt 6.6+ requirement
    
    # Enable multi-ABI support for broader device compatibility
    if(NOT DEFINED QT_ANDROID_BUILD_ALL_ABIS)
        set(QT_ANDROID_BUILD_ALL_ABIS ON)
    endif()
    
    # Configure supported ABIs
    # Can be overridden from command line with -DQT_ANDROID_ABIS="arm64-v8a"
    if(NOT DEFINED QT_ANDROID_ABIS)
        set(QT_ANDROID_ABIS "arm64-v8a;armeabi-v7a;x86_64;x86")
    endif()
    
    # Enable 16KB page size support for Android 16+
    # Qt 6.8+ automatically handles 16KB page size compatibility
    set(QT_ANDROID_16KB_PAGE_SIZE_SUPPORT ON)
    
    message(STATUS "Android configuration:")
    message(STATUS "  Package source: ${QT_ANDROID_PACKAGE_SOURCE_DIR}")
    message(STATUS "  Target SDK: ${QT_ANDROID_TARGET_SDK_VERSION}")
    message(STATUS "  Min SDK: ${QT_ANDROID_MIN_SDK_VERSION}")
    message(STATUS "  Multi-ABI: ${QT_ANDROID_BUILD_ALL_ABIS}")
    message(STATUS "  Supported ABIs: ${QT_ANDROID_ABIS}")
    message(STATUS "  16KB page size support: ${QT_ANDROID_16KB_PAGE_SIZE_SUPPORT}")
endif()

# Main executable target configuration
if(ANDROID)
    qt_add_executable(orayta main.cpp)
else()
    add_executable(orayta main.cpp)
endif()

# Link core libraries
target_link_libraries(orayta PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::PrintSupport
    OraytaBase
)

# Conditionally link UI libraries based on platform
if(MOBILE)
    target_link_libraries(orayta PRIVATE MobileUI)
    
    # Android-specific linking
    if(ANDROID)
        # Link CorePrivate for Android-specific APIs and JNI
        target_link_libraries(orayta PRIVATE Qt6::CorePrivate)
    endif()
else()
    # Desktop UI disabled - requires Qt5→Qt6 migration
    # target_link_libraries(orayta PRIVATE DesktopUI)
    message(STATUS "Desktop UI disabled - core library only")
endif()

# Add main resource file
# In Qt6, resources should be added directly to the target
target_sources(orayta PRIVATE
    Orayta.qrc
)

# Platform-specific executable configuration
if(WIN32)
    # Windows: Add RC file for application icon and version info
    target_sources(orayta PRIVATE orayta.rc)
    
elseif(ANDROID)
    # Android: Set package properties on the executable target
    # For multi-ABI builds, we need to specify OpenSSL for each ABI
    set(OPENSSL_LIBS "")
    foreach(abi IN LISTS QT_ANDROID_ABIS)
        list(APPEND OPENSSL_LIBS 
            "${CMAKE_CURRENT_SOURCE_DIR}/openssl-android/${abi}/libssl_3.so"
            "${CMAKE_CURRENT_SOURCE_DIR}/openssl-android/${abi}/libcrypto_3.so"
        )
    endforeach()
    
    set_target_properties(orayta PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${QT_ANDROID_PACKAGE_SOURCE_DIR}"
        QT_ANDROID_TARGET_SDK_VERSION "${QT_ANDROID_TARGET_SDK_VERSION}"
        QT_ANDROID_MIN_SDK_VERSION "${QT_ANDROID_MIN_SDK_VERSION}"
        QT_ANDROID_EXTRA_LIBS "${OPENSSL_LIBS}"
    )
    
    # Copy translation files to Android assets (only to Orayta subfolder where app reads them)
    add_custom_command(TARGET orayta POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/Hebrew.qm
            ${QT_ANDROID_PACKAGE_SOURCE_DIR}/assets/Orayta/Hebrew.qm
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/French.qm
            ${QT_ANDROID_PACKAGE_SOURCE_DIR}/assets/Orayta/French.qm
        COMMENT "Copying translation files to Android assets"
    )
    
    # Enable multi-ABI builds
    if(QT_ANDROID_BUILD_ALL_ABIS)
        set_target_properties(orayta PROPERTIES
            QT_ANDROID_BUILD_ALL_ABIS ON
        )
    endif()
    
elseif(UNIX AND NOT APPLE)
    # Linux: Set executable properties
    set_target_properties(orayta PROPERTIES
        OUTPUT_NAME orayta
    )
endif()

# Set general executable properties
if(MOBILE OR ANDROID)
    set_target_properties(orayta PROPERTIES
        WIN32_EXECUTABLE TRUE
        MACOSX_BUNDLE TRUE
    )
else()
    # For desktop builds without UI, keep console window
    set_target_properties(orayta PROPERTIES
        WIN32_EXECUTABLE FALSE
        MACOSX_BUNDLE FALSE
    )
endif()

# Qt6 finalization is handled automatically by qt_add_executable for Android

# Installation configuration (from original QMake setup)
if(NOT ANDROID)
    # Install binary
    install(TARGETS orayta
        RUNTIME DESTINATION bin
        BUNDLE DESTINATION .
    )
    
    # Install resources and data files
    if(WIN32)
        set(INSTALL_PATH ".")
    elseif(UNIX)
        set(INSTALL_PATH "share/Orayta")
    endif()
    
    # Install icon and images
    install(FILES Icons/Orayta.png Images/Wait.gif
        DESTINATION ${INSTALL_PATH}
    )
    
    # Install CSS files
    install(DIRECTORY css/
        DESTINATION ${INSTALL_PATH}
    )
    
    # Install translation files
    install(FILES Hebrew.qm
        DESTINATION ${INSTALL_PATH}
    )
    
    # Linux-specific installations
    if(UNIX AND NOT APPLE)
        # Install desktop file
        install(FILES Orayta.desktop
            DESTINATION share/applications
        )
    endif()
endif()