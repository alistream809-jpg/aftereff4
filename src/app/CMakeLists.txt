add_executable(AfterTrue WIN32 main.cpp ${CMAKE_SOURCE_DIR}/resources/app.rc)
target_link_libraries(AfterTrue PRIVATE at_ui)
