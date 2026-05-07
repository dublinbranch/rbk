# Other rbk tests (`filefunction.cpp`, …); Boost.JSON suites live under BoostJson/test/.
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../BoostJson/test" rbk_boostjson_tests_build)

add_executable(rbk_tests
    filefunction.cpp
    $<TARGET_OBJECTS:rbk_boostjson_test_objs>
)

target_link_libraries(rbk_tests
    PRIVATE
        rbk::rbk
        Boost::unit_test_framework
)

set_target_properties(rbk_tests PROPERTIES CXX_EXTENSIONS ON)
target_compile_features(rbk_tests PRIVATE cxx_std_26)

add_test(NAME rbk_tests COMMAND rbk_tests)
