#
#  Author
#      luncliff@gmail.com
#
cmake_minimum_required(VERSION 3.6)
project(ndk_camera  LANGUAGES CXX   VERSION 0.4)

if(NOT ANDROID) # Ensure android
    message(FATAL_ERROR "this module is only for Android platform")
    return(1)
endif()
set(BUILD_SHARED_LIBS true)

add_subdirectory(spdlog)
add_subdirectory(coroutine)

message(STATUS "========== ${PROJECT_NAME} : ${PROJECT_VERSION} ==========")
message(STATUS "System      \t: ${CMAKE_SYSTEM}")
message(STATUS "Build Type  \t: ${CMAKE_BUILD_TYPE}")
message(STATUS "Path")
message(STATUS "  Root      \t: ${CMAKE_SOURCE_DIR}")
message(STATUS "  Project   \t: ${PROJECT_SOURCE_DIR}")
message(STATUS "  Install   \t: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "CMake")
message(STATUS "  Version   \t: ${CMAKE_VERSION}")
message(STATUS "  Toolchain \t: ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "Compiler")
message(STATUS "  ID        \t: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  Version   \t: ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Path      \t: ${CMAKE_CXX_COMPILER}")

message(STATUS "Android")
message(STATUS "  Platform\t: ${ANDROID_PLATFORM}")     # API level
message(STATUS "  Arch    \t: ${ANDROID_ARCH_NAME}")    # 
message(STATUS "  ABI     \t: ${ANDROID_ABI}")          # 
message(STATUS "  NDK Path\t: ${ANDROID_NDK}")          # Path/to/NDK
message(STATUS "  STL     \t: ${ANDROID_STL}")          # expect c++_shared

if(${ANDROID_ABI} MATCHES "arm")
message(STATUS "ARM")
message(STATUS "  Mode     \t: ${ANDROID_ARM_MODE}")
message(STATUS "  Neon     \t: ${ANDROID_ARM_NEON}")
endif()


add_library(${PROJECT_NAME}
    include/ndk_camera.h
    src/libmain.cpp
)
set_target_properties(${PROJECT_NAME}
PROPERTIES
    CXX_STANDARD 14
)

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
    target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${ANDROID_NDK}/sources/cxx-stl/gnu-libstdc++/include/ 
    )
# STL: C++ Shared
elseif(${ANDROID_STL} STREQUAL "c++_shared")
    target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${ANDROID_NDK}/sources/cxx-stl/llvm-libc++/include/ 
    )
endif()

target_include_directories(${PROJECT_NAME}
PRIVATE
    # JNI
    ${ANDROID_NDK}/sysroot/usr/include/ 
    ${ANDROID_NDK}/sources/android/support/include/
    # ABI
    ${ANDROID_NDK}/platforms/${ANDROID_PLATFORM}/arch-${ANDROID_ARCH_NAME}/usr/include/ 
)

# Code for JNI
target_sources(${PROJECT_NAME}
PRIVATE
    android/jni/adapter.h
    android/jni/adapter.cpp
)

# Make <ndk_camera.h> visible
target_include_directories(${PROJECT_NAME}
PUBLIC
    include
PRIVATE
    android/jni
)

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

# https://developer.android.com/ndk/guides/stable_apis?hl=ko
target_link_libraries(${PROJECT_NAME} 
PUBLIC
    ${CMAKE_DL_LIBS} 
    android camera2ndk mediandk log
    spdlog::spdlog coro::portable
)
