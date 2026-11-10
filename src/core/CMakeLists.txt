add_library(at_core STATIC Json.cpp Id.cpp Timecode.cpp FileUtil.cpp)
target_include_directories(at_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
