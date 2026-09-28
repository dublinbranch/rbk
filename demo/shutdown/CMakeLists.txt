# Demo for rbk::Shutdown (thread/shutdown.h). Run demo.sh next to this file.
add_executable(rbk_shutdown_demo main.cpp)

target_link_libraries(rbk_shutdown_demo PRIVATE rbk::rbk)

set_target_properties(rbk_shutdown_demo PROPERTIES CXX_EXTENSIONS ON)
target_compile_features(rbk_shutdown_demo PRIVATE cxx_std_26)
