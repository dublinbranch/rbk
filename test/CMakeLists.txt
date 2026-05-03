add_executable(rbk_tests
    boost_test_main.cpp
    filefunction.cpp
    boostjson_parse.cpp
    boostjson_math.cpp
    boostjson_util.cpp
    boostjson_extra.cpp
    boostjson_pointer.cpp
)

target_link_libraries(rbk_tests
    PRIVATE
        rbk::rbk
        Boost::unit_test_framework
)

set_target_properties(rbk_tests PROPERTIES CXX_EXTENSIONS ON)
target_compile_features(rbk_tests PRIVATE cxx_std_26)

add_test(NAME rbk_tests COMMAND rbk_tests)
