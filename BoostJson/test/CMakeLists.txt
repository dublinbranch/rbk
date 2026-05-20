# Boost.JSON Boost.Test translation units + shared rbk_tests module entry (BOOST_TEST_MODULE).
# Consumed by rbk/test/CMakeLists.txt via $<TARGET_OBJECTS:…>.
add_library(rbk_boostjson_test_objs OBJECT
    boost_test_main.cpp
    boostjson_parse.cpp
    boostjson_math.cpp
    boostjson_util.cpp
    boostjson_extra.cpp
    boostjson_pointer.cpp
    boostjson_intrusive.cpp
)

set_target_properties(rbk_boostjson_test_objs PROPERTIES CXX_EXTENSIONS ON)
target_compile_features(rbk_boostjson_test_objs PRIVATE cxx_std_26)
target_link_libraries(rbk_boostjson_test_objs
    PRIVATE
        rbk::rbk
        Boost::unit_test_framework
)
