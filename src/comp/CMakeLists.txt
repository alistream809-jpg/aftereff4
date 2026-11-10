find_package(Threads REQUIRED)
add_library(at_comp STATIC Compositor.cpp)
target_link_libraries(at_comp PUBLIC at_project Threads::Threads)
