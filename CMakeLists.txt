cmake_minimum_required(VERSION 3.20)
if(APPLE)
    project(FirebirdPlugin LANGUAGES CXX OBJCXX)
else()
    project(FirebirdPlugin LANGUAGES CXX)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(PLUGIN_NAME "FirebirdPlugin")

# ---------------------------------------------------------------------------
# Platform detection
# ---------------------------------------------------------------------------
if(APPLE)
    set(FB_FRAMEWORK "/Library/Frameworks/Firebird.framework")
    set(FB_INCLUDE_DIR "${FB_FRAMEWORK}/Versions/A/Headers")
    set(FB_LIB_DIR "${FB_FRAMEWORK}/Versions/A/Libraries")
    set(PLATFORM_DEFINES COCOA=1 TARGET_COCOA=1 TARGET_CARBON=1 TARGET_MACOS=1)
    set(PLUGIN_EXT "dylib")
elseif(WIN32)
    # Firebird Windows: set via FIREBIRD_ROOT or auto-detect
    if(NOT DEFINED FIREBIRD_ROOT)
        # Try common install locations
        foreach(_fb_path
            "C:/Program Files/Firebird/Firebird_6_0"
            "C:/Program Files/Firebird/Firebird_5_0"
            "C:/Program Files/Firebird/Firebird_4_0"
            "C:/Firebird"
            "$ENV{FIREBIRD_ROOT}")
            if(EXISTS "${_fb_path}/include/ibase.h")
                set(FIREBIRD_ROOT "${_fb_path}")
                break()
            endif()
        endforeach()
    endif()
    if(NOT FIREBIRD_ROOT)
        message(FATAL_ERROR "Firebird not found. Set -DFIREBIRD_ROOT=<path>")
    endif()
    set(FB_INCLUDE_DIR "${FIREBIRD_ROOT}/include")
    set(FB_LIB_DIR "${FIREBIRD_ROOT}/lib")
    set(PLATFORM_DEFINES WINDOWS=1 TARGET_WIN32=1)
    set(PLUGIN_EXT "dll")
elseif(UNIX)
    # Linux: Firebird headers from system or FIREBIRD_ROOT
    if(NOT DEFINED FIREBIRD_ROOT)
        foreach(_fb_path "/usr" "/opt/firebird" "/usr/local")
            if(EXISTS "${_fb_path}/include/firebird/ibase.h" OR EXISTS "${_fb_path}/include/ibase.h")
                set(FIREBIRD_ROOT "${_fb_path}")
                break()
            endif()
        endforeach()
    endif()
    if(NOT FIREBIRD_ROOT)
        message(FATAL_ERROR "Firebird not found. Set -DFIREBIRD_ROOT=<path>")
    endif()
    if(EXISTS "${FIREBIRD_ROOT}/include/firebird")
        set(FB_INCLUDE_DIR "${FIREBIRD_ROOT}/include/firebird")
    else()
        set(FB_INCLUDE_DIR "${FIREBIRD_ROOT}/include")
    endif()
    set(FB_LIB_DIR "${FIREBIRD_ROOT}/lib")
    set(PLATFORM_DEFINES UNIX_ANSI=1 X_WINDOW=1 LINUX=1)
    set(PLUGIN_EXT "so")
endif()

message(STATUS "Platform: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Firebird include: ${FB_INCLUDE_DIR}")
message(STATUS "Firebird lib: ${FB_LIB_DIR}")

# ---------------------------------------------------------------------------
# Source files
# ---------------------------------------------------------------------------
set(SOURCES
    sources/FirebirdDB.cpp
    sources/FirebirdPlugin.cpp
    sdk/GlueCode/PluginMain.cpp
)

# macOS needs the Cocoa glue
if(APPLE)
    list(APPEND SOURCES sdk/GlueCode/PluginMainCocoa.mm)
endif()

# ---------------------------------------------------------------------------
# Shared library target
# ---------------------------------------------------------------------------
add_library(${PLUGIN_NAME} SHARED ${SOURCES})

target_include_directories(${PLUGIN_NAME} PRIVATE
    sdk/Includes
    ${FB_INCLUDE_DIR}
)

target_compile_definitions(${PLUGIN_NAME} PRIVATE ${PLATFORM_DEFINES})

# ---------------------------------------------------------------------------
# Platform-specific flags and linking
# ---------------------------------------------------------------------------
if(APPLE)
    target_compile_options(${PLUGIN_NAME} PRIVATE
        -fvisibility=hidden
        -Wno-deprecated-declarations
        -Wno-nullability-completeness
    )
    # Only export REALPluginMain
    file(WRITE "${CMAKE_BINARY_DIR}/exports.txt" "_REALPluginMain\n")
    set_target_properties(${PLUGIN_NAME} PROPERTIES
        LINK_FLAGS "-exported_symbols_list \"${CMAKE_BINARY_DIR}/exports.txt\""
    )

    target_link_directories(${PLUGIN_NAME} PRIVATE ${FB_LIB_DIR})
    target_link_libraries(${PLUGIN_NAME} PRIVATE
        fbclient
        "-framework Carbon"
        "-framework Cocoa"
        "-framework CoreFoundation"
    )
    # Build universal binary (arm64 + x86_64) if requested
    if(CMAKE_OSX_ARCHITECTURES)
        # Already set by user or CI
    else()
        set_target_properties(${PLUGIN_NAME} PROPERTIES
            OSX_ARCHITECTURES "${CMAKE_SYSTEM_PROCESSOR}"
        )
    endif()

elseif(WIN32)
    target_compile_definitions(${PLUGIN_NAME} PRIVATE
        _CRT_SECURE_NO_WARNINGS
        _CRT_NONSTDC_NO_DEPRECATE
        WIN32_LEAN_AND_MEAN
    )
    # Static CRT (/MT) — avoids requiring vcruntime DLLs on end-user machines
    set_property(TARGET ${PLUGIN_NAME} PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
    # MSVC: export only REALPluginMain via .def file
    set(DEF_FILE "${CMAKE_BINARY_DIR}/FirebirdPlugin.def")
    file(WRITE ${DEF_FILE} "LIBRARY ${PLUGIN_NAME}\nEXPORTS\n    REALPluginMain\n")
    set_target_properties(${PLUGIN_NAME} PROPERTIES
        LINK_FLAGS "/DEF:${DEF_FILE}"
    )
    # Find fbclient import library
    find_library(FBCLIENT_LIB
        NAMES fbclient_ms fbclient
        PATHS ${FB_LIB_DIR}
    )
    if(NOT FBCLIENT_LIB)
        message(FATAL_ERROR "fbclient library not found in ${FB_LIB_DIR}")
    endif()
    target_link_libraries(${PLUGIN_NAME} PRIVATE ${FBCLIENT_LIB})

elseif(UNIX)
    target_compile_options(${PLUGIN_NAME} PRIVATE
        -fPIC -fvisibility=hidden -Wno-narrowing
    )
    # Version script to export only REALPluginMain
    set(VERSION_SCRIPT "${CMAKE_BINARY_DIR}/exports.map")
    file(WRITE ${VERSION_SCRIPT} "{ global: REALPluginMain; local: *; };\n")
    set_target_properties(${PLUGIN_NAME} PROPERTIES
        LINK_FLAGS "-Wl,--version-script=${VERSION_SCRIPT}"
    )
    target_link_directories(${PLUGIN_NAME} PRIVATE ${FB_LIB_DIR})
    target_link_libraries(${PLUGIN_NAME} PRIVATE fbclient)
endif()

# Remove "lib" prefix on all platforms
set_target_properties(${PLUGIN_NAME} PROPERTIES PREFIX "")

# ---------------------------------------------------------------------------
# Package target: assemble .xojo_plugin ZIP
# ---------------------------------------------------------------------------
# Determine the Xojo platform folder name
if(APPLE)
    if(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64;x86_64" OR
       CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64;arm64")
        set(XOJO_PLATFORM "Mac Universal")
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64" OR
           CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
        set(XOJO_PLATFORM "Mac arm64")
    else()
        set(XOJO_PLATFORM "Mac x86-64")
    endif()
elseif(WIN32)
    # CMAKE_GENERATOR_PLATFORM is set by -A flag (x64, ARM64, Win32)
    if(CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64")
        set(XOJO_PLATFORM "Windows arm64")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(XOJO_PLATFORM "Windows x86-64")
    else()
        set(XOJO_PLATFORM "Windows")
    endif()
elseif(UNIX)
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        set(XOJO_PLATFORM "Linux x86-64")
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
        set(XOJO_PLATFORM "Linux arm64")
    else()
        set(XOJO_PLATFORM "Linux X86")
    endif()
endif()

message(STATUS "Xojo platform folder: ${XOJO_PLATFORM}")

add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "${CMAKE_BINARY_DIR}/plugin/${PLUGIN_NAME}/Build Resources/${XOJO_PLATFORM}"
    COMMAND ${CMAKE_COMMAND} -E copy
        "$<TARGET_FILE:${PLUGIN_NAME}>"
        "${CMAKE_BINARY_DIR}/plugin/${PLUGIN_NAME}/Build Resources/${XOJO_PLATFORM}/"
    COMMAND ${CMAKE_COMMAND} -E touch
        "${CMAKE_BINARY_DIR}/plugin/Version.info"
    COMMENT "Staged plugin binary for ${XOJO_PLATFORM}"
)

# Create .xojo_plugin ZIP archive
add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E tar cf
        "${CMAKE_BINARY_DIR}/${PLUGIN_NAME}.xojo_plugin"
        --format=zip
        "${PLUGIN_NAME}" "Version.info"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/plugin"
    COMMENT "Packaged ${PLUGIN_NAME}.xojo_plugin"
)
