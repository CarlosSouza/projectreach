cmake_minimum_required(VERSION 3.24)
project(HaloPadAngle LANGUAGES C CXX OBJCXX)

# A bounded iOS build using WebKit's maintained ANGLE source lists.
# Source is fetched separately into scratch; no third-party source is vendored.
string(TOLOWER "${CMAKE_OSX_SYSROOT}" angle_sdk)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS" OR NOT angle_sdk MATCHES "(^|/)(iphoneos|iphonesimulator)([0-9.]+\\.sdk)?$")
    message(FATAL_ERROR "This experimental build requires the iOS device or Simulator SDK")
endif()
if(NOT EXISTS "${ANGLE_SOURCE_DIR}/PlatformCocoa.cmake")
    message(FATAL_ERROR "ANGLE_SOURCE_DIR must be WebKit's Source/ThirdParty/ANGLE")
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OBJCXX_STANDARD 20)
set(is_apple TRUE)
set(is_ios TRUE)
set(WEBKIT_SDK_IS_MACOS FALSE)
include("${ANGLE_SOURCE_DIR}/Compiler.cmake")
include("${ANGLE_SOURCE_DIR}/GLESv2.cmake")
include("${ANGLE_SOURCE_DIR}/Metal.cmake")

set(ANGLE_SOURCES
    ${libangle_common_sources} ${libangle_common_shader_state_sources}
    ${angle_preprocessor_sources} ${angle_translator_sources}
    ${angle_translator_essl_symbol_table_sources} ${angle_translator_essl_sources}
    ${angle_translator_glsl_base_sources} ${angle_translator_glsl_and_vulkan_base_sources}
    ${angle_translator_glsl_sources} ${angle_translator_glsl_apple_sources}
    ${libangle_sources} ${libangle_image_util_sources} ${xxhash_sources}
    ${libglesv2_entry_point_sources} ${libglesv2_sources} ${libegl_sources}
    src/common/angle_version_info.cpp
    src/libANGLE/capture/FrameCapture_mock.cpp
    src/libANGLE/capture/serialize_mock.cpp
    third_party/zlib/google/compression_utils_portable.cc
)
set(ANGLE_DEFINITIONS ANGLE_ENABLE_ESSL ANGLE_ENABLE_GLSL)
include("${ANGLE_SOURCE_DIR}/PlatformCocoa.cmake")
list(REMOVE_DUPLICATES ANGLE_SOURCES)
list(TRANSFORM ANGLE_SOURCES PREPEND "${ANGLE_SOURCE_DIR}/")
add_library(halopad-angle STATIC ${ANGLE_SOURCES})
target_include_directories(halopad-angle PUBLIC "${ANGLE_SOURCE_DIR}/include" PRIVATE
    "${ANGLE_SOURCE_DIR}/WebKit" "${ANGLE_SOURCE_DIR}/src"
    "${ANGLE_SOURCE_DIR}/src/common/base"
    "${ANGLE_SOURCE_DIR}/src/common/third_party/xxhash"
    "${ANGLE_SOURCE_DIR}/third_party/zlib/google")
target_compile_definitions(halopad-angle PRIVATE ${ANGLE_DEFINITIONS}
    LIBANGLE_IMPLEMENTATION LIBGLESV2_IMPLEMENTATION USE_SYSTEM_ZLIB
    GL_GLES_PROTOTYPES=0 EGL_EGL_PROTOTYPES=0)
target_compile_options(halopad-angle PRIVATE -w)
target_link_libraries(halopad-angle PUBLIC ${ANGLEGLESv2_LIBRARIES} ZLIB::ZLIB)
