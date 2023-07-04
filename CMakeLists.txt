cmake_minimum_required(VERSION 3.18)
project(SpaceCombatGame VERSION 0.0.1 LANGUAGES CXX)


# Configuration ########################################################################################################
option(BUILD_SHARED_LIBS "Build all libaries as shared" OFF)

set(ENGINE_RENDERER VULKAN CACHE STRING "Renderer API")
set_property(CACHE ENGINE_RENDERER PROPERTY STRINGS METAL OPENGL VULKAN)

# CXX Standard and Runtime #############################################################################################
set(CMAKE_CXX_EXTENSIONS OFF)
if (NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif ()

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")


# Install CPM ##########################################################################################################
set(CPM_VERSION 0.38.1)
set(CPM_SOURCE_CACHE ${CMAKE_SOURCE_DIR}/cache)
set(CPM_DOWNLOAD_LOCATION "${CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

if (NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
    file(DOWNLOAD https://github.com/TheLartians/CPM.cmake/releases/download/v${CPM_VERSION}/CPM.cmake
            ${CPM_DOWNLOAD_LOCATION})
endif ()

include(${CPM_DOWNLOAD_LOCATION})

## Install Crinkler ####################################################################################################
if(MSVC)
    set(CRINKLER_DOWNLOAD_LOCATION ${CMAKE_BINARY_DIR}/crinkler)

    if(NOT (EXISTS ${CRINKLER_DOWNLOAD_LOCATION}))
        file(DOWNLOAD https://github.com/runestubbe/Crinkler/releases/download/v2.3/crinkler23.zip ${CRINKLER_DOWNLOAD_LOCATION}/crinkler.zip)
        file(ARCHIVE_EXTRACT INPUT ${CRINKLER_DOWNLOAD_LOCATION}/crinkler.zip DESTINATION ${CRINKLER_DOWNLOAD_LOCATION})
    endif()
endif()


# Project Include Directories ##########################################################################################
set(PROJECT_INCLUDE_DIRECTORIES source)


# Project Compile Definitions ###########################################################################################
set(PROJECT_COMPILE_DEFINITIONS)

if(WIN32)
    message("Using Windows Platform")
    list(APPEND PROJECT_COMPILE_DEFINITIONS
            PLATFORM_WINDOWS
            NOMINMAX
            VC_EXTRALEAN
            WIN32_LEAN_AND_MEAN)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    list(APPEND PROJECT_COMPILE_DEFINITIONS
            PLATFORM_LINUX)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    list(APPEND PROJECT_COMPILE_DEFINITIONS
            PLATFORM_MACOS)
endif()


# Project Compile Features #############################################################################################
set(PROJECT_COMPILE_FEATURES cxx_std_20)


# Project Compile Warnings #############################################################################################
set(MSVC_WARNINGS
        /W4     # Baseline reasonable warnings
        /w14242 # 'identifier': conversion from 'type1' to 'type1', possible loss of data
        /w14254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
        /w14263 # 'function': member function does not override any base class virtual member function
        /w14265 # 'classname': class has virtual functions, but destructor is not
        /w14287 # 'operator': unsigned/negative constant mismatch
        /we4289 # nonstandard extension used: 'variable': loop control variable used outside the loop scope
        /w14296 # 'operator': expression is always 'boolean_value'
        /w14311 # 'variable': pointer truncation from 'type1' to 'type2'
        /w14545 # expression before comma evaluates to a function which is missing an argument list
        /w14546 # function call before comma missing argument list
        /w14547 # 'operator': operator before comma has no effect; expected operator with side-effect
        /w14549 # 'operator': operator before comma has no effect; did you intend 'operator'?
        /w14555 # expression has no effect; expected expression with side- effect
        /w14619 # pragma warning: there is no warning number 'number'
        /w14640 # Enable warning on thread un-safe static member initialization
        /w14826 # Conversion from 'type1' to 'type_2' is sign-extended. This may cause unexpected runtime behavior
        /w14905 # wide string_t literal cast to 'LPSTR'
        /w14906 # string_t literal cast to 'LPWSTR'
        /w14928 # illegal copy-initialization; more than one user-defined conversion has been implicitly applied
        /permissive-) # standards conformance mode for MSVC compiler

set(CLANG_WARNINGS
        -Wall
        -Wextra  # reasonable and standard
        -Wshadow # warn the user if a variable declaration shadows one from a parent context
        -Wnon-virtual-dtor # warn the user if a class with virtual functions has a non-virtual destructor
        -Wold-style-cast # warn for c-style casts
        -Wcast-align # warn for potential performance problem casts
        -Wunused # warn on anything being unused
        -Woverloaded-virtual # warn if you overload (not override) a virtual function
        -Wpedantic # warn if non-standard C++ is used
        -Wconversion # warn on type conversions that may lose data
        -Wsign-conversion # warn on sign conversions
        -Wnull-dereference # warn if a null dereference is detected
        -Wdouble-promotion # warn if float is implicit promoted to double
        -Wformat=2) # warn on security issues around functions that format output (ie printf)

set(GCC_WARNINGS
        ${CLANG_WARNINGS}
        -Wmisleading-indentation # warn if indentation implies blocks where blocks # do not exist
        -Wduplicated-cond # warn if if / else chain has duplicated conditions
        -Wduplicated-branches # warn if if / else branches have duplicated code
        -Wlogical-op   # warn about logical operations being used where bitwise were probably wanted
        -Wuseless-cast) # warn if you perform a cast to the same type

if(MSVC)
    set(PROJECT_COMPILE_WARNINGS ${MSVC_WARNINGS})
elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    set(PROJECT_COMPILE_WARNINGS ${CLANG_WARNINGS})
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(PROJECT_COMPILE_WARNINGS ${GCC_WARNINGS})
endif()

# Project Link Options #################################################################################################
set(MSVC_LINK_OPTIONS
        /NODEFAULTLIB
        /MANIFEST:NO
        /ENTRY:entry)

set(CLANG_GCC_LINK_OPTIONS
        -e _entry
        -nostdlib
        -nostartfiles
        -nodefaultlibs
        -fno-stack-protector)

if(MSVC)
    set(PROJECT_LINK_OPTIONS ${MSVC_LINK_OPTIONS})
    if(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
        set(CMAKE_LINKER ${CRINKLER_DOWNLOAD_LOCATION}/crinkler23/win64/Crinkler.exe)
        list(APPEND PROJECT_LINK_OPTIONS /CRINKLER)
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(PROJECT_LINK_OPTIONS ${CLANG_GCC_LINK_OPTIONS})
endif()


# Project Compile Options ##############################################################################################
set(MSVC_COMPILE_OPTIONS
        /Gs10000000 # Disable stack probes
        /sdl-       # Disable recommended security checks
        /GS-        # Disable buffer security checks
        /Oi         # Generate intrinsic functions
        /Gz         # Use __stdcall convention
        /Oy         # Omit frame pointers
        /EHa-       # Disable exceptions
        /arch:SSE2) # Enable SSE2)

set(CLANG_GCC_COMPILE_OPTIONS
        -fno-exceptions
        -fno-rtti)

if(MSVC)
    set(PROJECT_COMPILE_OPTIONS ${MSVC_COMPILE_OPTIONS})

    # Clear /RTC runtime checks
    set(CompilerFlags
            CMAKE_C_FLAGS
            CMAKE_CXX_FLAGS
            CMAKE_C_FLAGS_DEBUG
            CMAKE_CXX_FLAGS_DEBUG
            CMAKE_C_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_RELEASE)
    foreach(CompilerFlag ${CompilerFlags})
        string(REPLACE
                "/GR" "/GR-" ${CompilerFlag} "${${CompilerFlag}}")
        string(REGEX REPLACE
                "/RTC(su|[1su])" "" ${CompilerFlag} "${${CompilerFlag}}")
    endforeach()
elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(PROJECT_COMPILE_OPTIONS ${CLANG_GCC_COMPILE_OPTIONS})
endif()

list(APPEND PROJECT_COMPILE_OPTIONS ${PROJECT_COMPILE_WARNINGS})


# Build Engine #########################################################################################################

# Core
add_library(core INTERFACE)
target_include_directories(core INTERFACE ${PROJECT_INCLUDE_DIRECTORIES})
target_compile_definitions(core INTERFACE ${PROJECT_COMPILE_DEFINITIONS})
target_compile_features(core INTERFACE ${PROJECT_COMPILE_FEATURES})
target_compile_options(core INTERFACE ${PROJECT_COMPILE_OPTIONS})
target_link_options(core INTERFACE ${PROJECT_LINK_OPTIONS})
target_sources(core INTERFACE
        source/engine/core/types.h
        source/engine/core/array.h
        source/engine/core/hash.h
        source/engine/core/logger.h
        source/engine/core/string.h)


# Platform
add_library(platform)
target_include_directories(platform PRIVATE ${PROJECT_INCLUDE_DIRECTORIES})
target_compile_definitions(platform PRIVATE ${PROJECT_COMPILE_DEFINITIONS})
target_compile_features(platform PRIVATE ${PROJECT_COMPILE_FEATURES})
target_compile_options(platform PRIVATE ${PROJECT_COMPILE_OPTIONS})
target_link_options(platform PRIVATE ${PROJECT_LINK_OPTIONS})
target_sources(platform PRIVATE
        source/engine/platform/platform_system.cpp
        source/engine/platform/platform_system.h
        source/engine/platform/platform_types.h)
if(WIN32)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    find_package(X11 REQUIRED)
    include_directories(${X11_INCLUDE_DIR})
    target_link_libraries(platform PRIVATE ${X11_LIBRARIES})
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_library(COCOA_LIBRARY Cocoa)
    target_link_libraries(platform PRIVATE dl ${COCOA_LIBRARY})
endif()


# Renderer
add_library(renderer)
target_include_directories(renderer PRIVATE ${PROJECT_INCLUDE_DIRECTORIES})
target_compile_definitions(renderer PRIVATE ${PROJECT_COMPILE_DEFINITIONS})
target_compile_features(renderer PRIVATE ${PROJECT_COMPILE_FEATURES})
target_compile_options(renderer PRIVATE ${PROJECT_COMPILE_OPTIONS})
target_link_options(renderer PRIVATE ${PROJECT_LINK_OPTIONS})
target_sources(renderer PRIVATE
        source/engine/renderer/renderer_system.cpp
        source/engine/renderer/renderer_system.h
        source/engine/renderer/renderer_types.h)
if(ENGINE_RENDERER STREQUAL METAL)
    message("Using Metal Renderer")
    find_library(METAL_LIBRARY Metal)
    target_link_libraries(renderer PRIVATE ${METAL_LIBRARY})
    target_compile_definitions(renderer PRIVATE RENDERER_METAL)
    if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        enable_language(OBJCXX)
        set_source_files_properties(source/engine/renderer/renderer_system.cpp PROPERTIES LANGUAGE OBJCXX)
    endif()
elseif(ENGINE_RENDERER STREQUAL OPENGL)
    message("Using OpenGL Renderer")
    target_compile_definitions(renderer PRIVATE RENDERER_OPENGL)
elseif(ENGINE_RENDERER STREQUAL VULKAN)
    message("Using Vulkan Renderer")
    CPMAddPackage(gh:KhronosGroup/Vulkan-Headers@1.3.255)
    target_link_libraries(renderer PRIVATE Vulkan::Headers)
    target_compile_definitions(renderer PRIVATE
            RENDERER_VULKAN
            VK_NO_PROTOTYPES
            VK_ENABLE_BETA_EXTENSIONS)
    if(WIN32)
        target_compile_definitions(renderer PRIVATE VK_USE_PLATFORM_WIN32_KHR)
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        target_compile_definitions(renderer PRIVATE VK_USE_PLATFORM_XCB_KHR)
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        target_compile_definitions(renderer PRIVATE VK_USE_PLATFORM_METAL_EXT)
    endif()
endif()


# Build Game Library ###################################################################################################
add_executable(game WIN32)
target_include_directories(game PRIVATE ${PROJECT_INCLUDE_DIRECTORIES})
target_compile_definitions(game PRIVATE ${PROJECT_COMPILE_DEFINITIONS})
target_compile_features(game PRIVATE ${PROJECT_COMPILE_FEATURES})
target_compile_options(game PRIVATE ${PROJECT_COMPILE_OPTIONS})
target_link_libraries(game PRIVATE core platform renderer)
target_link_options(game PRIVATE ${PROJECT_LINK_OPTIONS})
target_sources(game PRIVATE
        source/game/game.cpp)
