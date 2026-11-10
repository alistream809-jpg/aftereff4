cmake_minimum_required(VERSION 3.21)
project(AfterTrue VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(AT_BUILD_APP "Build the Qt + FFmpeg desktop application" ON)
option(AT_BUILD_TESTS "Build the unit tests" ON)

if(MSVC)
  add_compile_options(/utf-8 /permissive- /Zc:__cplusplus /MP /W3)
  add_compile_definitions(NOMINMAX _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
endif()

# Platform-independent modules (no Qt, no FFmpeg) - unit tested
add_subdirectory(src/core)
add_subdirectory(src/anim)
add_subdirectory(src/project)
add_subdirectory(src/comp)

if(AT_BUILD_APP)
  set(CMAKE_AUTOMOC ON)
  find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets)

  # FFmpeg (installed by vcpkg, see vcpkg.json)
  find_path(FFMPEG_INCLUDE_DIR libavcodec/avcodec.h REQUIRED)
  set(FFMPEG_LIBRARIES "")
  foreach(lib avformat avcodec swscale swresample avutil)
    find_library(FFMPEG_${lib}_LIBRARY NAMES ${lib} REQUIRED)
    list(APPEND FFMPEG_LIBRARIES ${FFMPEG_${lib}_LIBRARY})
  endforeach()

  add_subdirectory(src/media)
  add_subdirectory(src/ui)
  add_subdirectory(src/app)
endif()

if(AT_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
