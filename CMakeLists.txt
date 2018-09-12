# ---------------------------------------------------------------------------
#
#  Author
#      luncliff@gmail.com
#
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.1)

# ------------ Dependency ------------

# add_subdirectory(spdlog) # header-only
add_subdirectory(cppmagic)

# ------------ Project Variables ------------

project(ndk_camera LANGUAGES CXX 
        VERSION 0.2)

# Ensure android
if(NOT ANDROID)
    message(FATAL_ERROR "this module is only for Android platform")
    return(1)
endif() # 

message(STATUS "========== ${PROJECT_NAME} : ${PROJECT_VERSION} ==========")

set(ROOT_DIR    ${CMAKE_SOURCE_DIR})
set(PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
message(STATUS "Root        \t: ${ROOT_DIR}")
message(STATUS "Project     \t: ${PROJECT_DIR}")
message(STATUS "Install     \t: ${CMAKE_INSTALL_PREFIX}")
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug) # Debug by default
endif()
message(STATUS "Build Type  \t: ${CMAKE_BUILD_TYPE}")

# Platform check
set(BUILD_SHARED_LIBS true)

message(STATUS "System      \t: ${CMAKE_SYSTEM}")
message(STATUS "Compiler")
message(STATUS "  ID        \t: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  Path      \t: ${CMAKE_CXX_COMPILER}")

# ------------ Global Setup ------------

# link_driectories must come before project creation
# link_directories(
#     ///
# )

# ------------ Create Project ------------

add_library(${PROJECT_NAME} 
    ${PROJECT_DIR}/include/ndk_camera.h

    ${PROJECT_DIR}/src/libmain.cpp
)
add_dependencies(${PROJECT_NAME} cppmagic)

message(STATUS "Android")
message(STATUS "  Platform\t: ${ANDROID_PLATFORM}")     # Android-21 ++
message(STATUS "  Arch    \t: ${ANDROID_ARCH_NAME}")    # arm
message(STATUS "  ABI     \t: ${ANDROID_ABI}")          # armeabi-v7a or else
message(STATUS "  NDK Path\t: ${ANDROID_NDK}")          # Path/to/NDK
message(STATUS "  STL     \t: ${ANDROID_STL}")          # expect c++_shared

# path for <asm/types.h>
if(${ANDROID_ABI} STREQUAL "arm64-v8a")
    target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${ANDROID_NDK}/sysroot/usr/include/aarch64-linux-android
    )
elseif(${ANDROID_ABI} MATCHES "arm")
    target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${ANDROID_NDK}/sysroot/usr/include/arm-linux-androideabi
    )
elseif(${ANDROID_ABI} MATCHES "x86") # x86, x86_64
    target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${ANDROID_NDK}/sysroot/usr/include/x86_64-linux-android
    )
endif()

# STL: GNU Shared
if(${ANDROID_STL} STREQUAL "gnustl_shared")
    message(FATAL_ERROR "This library doesn't support ${ANDROID_STL}")
    return(1)

    target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${ANDROID_NDK}/sources/cxx-stl/gnu-libstdc++/include/ 
    )
    # link_directories(
    #     ${ANDROID_NDK}/sources/cxx-stl/gnu-libstdc++/4.9/libs/${ANDROID_ABI}/
    # )
# STL: C++ Shared
elseif(${ANDROID_STL} STREQUAL "c++_shared")
    target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${ANDROID_NDK}/sources/cxx-stl/llvm-libc++/include/ 
    )
    # link_directories(
    #     ${ANDROID_NDK}/sources/cxx-stl/llvm-libc++/libs/${ANDROID_ABI}/
    # )
endif()

target_include_directories(${PROJECT_NAME}
PRIVATE
    # JNI
    ${ANDROID_NDK}/sysroot/usr/include/ 
    ${ANDROID_NDK}/sources/android/support/include/
    # ABI
    ${ANDROID_NDK}/platforms/${ANDROID_PLATFORM}/arch-${ANDROID_ARCH_NAME}/usr/include/ 
    # JNI declarations
    ${PROJECT_DIR}/android/jni/
)

# link_directories(
#     # ABI
#     ${ANDROID_NDK}/platforms/${ANDROID_PLATFORM}/arch-${ANDROID_ARCH_NAME}/usr/lib/    
#     # Nested libraries
#     ${PROJECT_DIR}/android/jniLibs/${ANDROID_ABI}
# )

# Code for JNI
target_sources(${PROJECT_NAME}
PRIVATE
    ${PROJECT_DIR}/android/jni/adapter.h
    ${PROJECT_DIR}/android/jni/adapter.cpp
)

# Make <ndk_camera.h> visible
target_include_directories(${PROJECT_NAME}
PUBLIC
    ${PROJECT_DIR}/include    
PRIVATE
    ${PROJECT_DIR}/spdlog/include    
)

# Compiler Flags
target_compile_options(${PROJECT_NAME}
PRIVATE
    -fvisibility=hidden
)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_options(${PROJECT_NAME}
    PRIVATE
        -g
    )
else()
    target_compile_options(${PROJECT_NAME}
    PRIVATE
        -O3
    )
endif()

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        FMT_HEADER_ONLY
)

# ------------ Linkage ------------

# https://developer.android.com/ndk/guides/stable_apis?hl=ko
target_link_libraries(${PROJECT_NAME} 
PUBLIC
    ${CMAKE_DL_LIBS} cppmagic
    android log camera2ndk
)

