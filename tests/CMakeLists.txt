add_executable(at_tests
  test_main.cpp test_core.cpp test_anim.cpp test_project.cpp test_compositor.cpp)
target_include_directories(at_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(at_tests PRIVATE at_comp at_project at_anim at_core)
add_test(NAME unit_tests COMMAND at_tests)
