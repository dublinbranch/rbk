cmake_minimum_required(VERSION 3.16)
project(beastStreaming LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Beast/Asio: headers. Boost.JSON implementation lives in src/boost_json.cpp
# (includes boost/json/src.hpp) so no separate libboost_json package is required.
find_package(Boost REQUIRED CONFIG COMPONENTS headers)
find_package(Threads REQUIRED)

add_executable(beast_http_server src/main.cpp src/boost_json.cpp)
target_link_libraries(beast_http_server PRIVATE Boost::headers Threads::Threads)
