add_library(at_media STATIC VideoReader.cpp MediaManager.cpp)
target_include_directories(at_media PRIVATE ${FFMPEG_INCLUDE_DIR})
target_link_libraries(at_media PUBLIC at_project ${FFMPEG_LIBRARIES})
