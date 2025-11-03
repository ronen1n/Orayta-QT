# OraytaBase Core Library
# Contains platform-independent core functionality for book management, 
# search, HTML generation, and compression

# Define the OraytaBase static library
add_library(OraytaBase STATIC
    # Core book management
    booklist.cpp
    book.cpp
    bookiter.cpp
    
    # Search functionality  
    search.cpp
    bookfind.cpp
    
    # HTML generation and utilities
    htmlgen.cpp
    functions.cpp
    guematria.cpp
    
    # Network and file operations
    filedownloader.cpp
    
    # Bookmark management
    bmarklist.cpp
    minibmark.cpp
    

)

# Add corresponding header files
target_sources(OraytaBase PRIVATE
    # Core headers
    booklist.h
    book.h
    bookiter.h
    search.h
    bookfind.h
    htmlgen.h
    functions.h
    guematria.h
    filedownloader.h
    bmarklist.h
    minibmark.h
    

)

# Process UI forms (Qt6 handles .ui files automatically when linked)
# The bookfind.ui will be processed automatically by Qt6's build system

# Link Qt6 libraries
target_link_libraries(OraytaBase PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)

# Platform-specific configurations
if(WIN32 OR ANDROID)
    # Windows and Android: Use static QuaZip with bundled zlib
    target_compile_definitions(OraytaBase PRIVATE QUAZIP_STATIC)
    
    # Add QuaZip sources
    target_sources(OraytaBase PRIVATE
        quazip/quazip.cpp
        quazip/zip.c
        quazip/unzip.c
        quazip/quazipnewinfo.cpp
        quazip/quazipfile.cpp
        quazip/JlCompress.cpp
        quazip/quacrc32.cpp
        quazip/quaadler32.cpp
        quazip/qioapi.cpp
        
        # QuaZip headers
        quazip/quazip.h
        quazip/zip.h
        quazip/unzip.h
        quazip/quazipnewinfo.h
        quazip/quazipfileinfo.h
        quazip/quazipfile.h
        quazip/quazip_global.h
        quazip/JlCompress.h
        quazip/ioapi.h
        quazip/crypt.h
        quazip/quacrc32.h
        quazip/quachecksum32.h
        quazip/quaadler32.h
    )
    
    # Suppress warnings for legacy C code in quazip (third-party library)
    # These files use old K&R C style function definitions
    set_source_files_properties(
        quazip/zip.c
        quazip/unzip.c
        PROPERTIES
        COMPILE_FLAGS "-Wno-deprecated-non-prototype -Wno-incompatible-pointer-types"
    )
    
    target_include_directories(OraytaBase PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/quazip)
    
    # Use zlib
    find_package(ZLIB REQUIRED)
    target_link_libraries(OraytaBase PRIVATE ZLIB::ZLIB)
    
    if(WIN32)
        message(STATUS "Using zlib from vcpkg")
    else()
        message(STATUS "Using zlib for Android")
    endif()
    
elseif(UNIX AND NOT APPLE)
    # Linux: Link system zlib
    find_package(ZLIB REQUIRED)
    target_link_libraries(OraytaBase PRIVATE ZLIB::ZLIB)
endif()

# Set include directories
target_include_directories(OraytaBase PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/quazip
    ${CMAKE_CURRENT_BINARY_DIR}  # For generated UI headers
    ${CMAKE_SOURCE_DIR}  # For Desktop/ includes
)

# Export OraytaBase for use by other targets
set_target_properties(OraytaBase PROPERTIES
    EXPORT_NAME OraytaBase
    OUTPUT_NAME oraytagase
)