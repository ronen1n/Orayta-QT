# Mobile UI Library
# Contains Android-specific UI components and JNI integration

# Define the Mobile UI static library
add_library(MobileUI STATIC
    # Core mobile UI components
    mobileapp.cpp
    textdisplayer.cpp
    swipegesturerecognizer.cpp
    
    # Mobile-specific functionality
    mobileapp_download.cpp
    mobileapp_bookmark.cpp
    
    # Permission utilities (conditional on Android)
    $<$<BOOL:${ANDROID}>:permissionutils.cpp>
    
    # JNI integration (conditional on KOOKITA)
    $<$<BOOL:${ANDROID}>:jnifunc.cpp>
    
    # Android 16 support utilities
    $<$<BOOL:${ANDROID}>:android16support.cpp>
    
    # Edge-to-edge display support
    $<$<BOOL:${ANDROID}>:edgetoedgesupport.cpp>
    
    # Theme support
    $<$<BOOL:${ANDROID}>:themesupport.cpp>
)

# Add corresponding header files
target_sources(MobileUI PRIVATE
    # Mobile UI headers
    mobileapp.h
    textdisplayer.h
    swipegesturerecognizer.h
    
    # Permission utilities headers (conditional on Android)
    $<$<BOOL:${ANDROID}>:permissionutils.h>
    
    # JNI headers (conditional on Android)
    $<$<BOOL:${ANDROID}>:jnifunc.h>
    
    # Android 16 support headers
    $<$<BOOL:${ANDROID}>:android16support.h>
    
    # Edge-to-edge display support headers
    $<$<BOOL:${ANDROID}>:edgetoedgesupport.h>
    
    # Theme support headers
    $<$<BOOL:${ANDROID}>:themesupport.h>
)

# Process UI forms (Qt6 handles .ui files automatically when linked)
# The mobileapp.ui will be processed automatically by Qt6's build system

# Add mobile-specific resource file
# In Qt6, resources should be added directly to the target
target_sources(MobileUI PRIVATE
    mobile.qrc
)

# Link Qt6 libraries
target_link_libraries(MobileUI PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    OraytaBase
)

# Android-specific configuration
if(ANDROID)
    # Link CorePrivate for JNI functionality
    target_link_libraries(MobileUI PRIVATE Qt6::CorePrivate)
    
    # Mobile and Android compile definitions
    target_compile_definitions(MobileUI PRIVATE
        MOBILE
        ANDROID
        KOOKITA
        QTSCROLLER_NO_WEBKIT
    )
    
    # Include Android JNI headers
    target_include_directories(MobileUI PRIVATE
        ${ANDROID_NDK}/sysroot/usr/include
    )
endif()

# Set include directories
target_include_directories(MobileUI PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Export MobileUI for use by main executable
set_target_properties(MobileUI PROPERTIES
    EXPORT_NAME MobileUI
    OUTPUT_NAME oraytamobile
)