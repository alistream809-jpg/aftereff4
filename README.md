# AFTER TRUE

A native Windows motion-graphics / video-compositing application inspired by the After Effects workflow.
**Status: Milestone 1 (v0.1.0) - a working foundation, not yet a full After Effects replacement.**

Beginner? Read [START_HERE.md](START_HERE.md). Honest feature list: [docs/FEATURES.md](docs/FEATURES.md).

## Stack
C++20 - Qt 6 (Widgets) - FFmpeg (libav*, LGPL, dynamically linked) - CMake - vcpkg - GitHub Actions - Inno Setup.

## Build locally (developers)
Requirements: Visual Studio 2022 (C++ workload), CMake 3.21+, Qt 6.5+ (MSVC 64-bit), vcpkg.
```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-windows -DCMAKE_PREFIX_PATH=C:/Qt/6.7.3/msvc2019_64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```
Logic-only modules (core, anim, project, comp) and the tests build with any C++20 compiler:
`cmake -S . -B build -DAT_BUILD_APP=OFF`.

Docs: [architecture](docs/ARCHITECTURE.md) - [project file format](docs/PROJECT_FORMAT.md) - [third-party licences](docs/THIRD_PARTY.md).
Licence: MIT (see LICENSE) for the AFTER TRUE source; third-party components keep their own licences.
