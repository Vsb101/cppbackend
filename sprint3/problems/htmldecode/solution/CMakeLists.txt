cmake_minimum_required(VERSION 3.11)

project(htmldecode CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

set(CATCH_INCLUDE_DIR "/home/vladimir/.conan/data/catch2/3.5.0/_/_/package/2d3baf6699d9c743852b6d73c81f1517997b5976/include")
include_directories(${CATCH_INCLUDE_DIR})

add_executable(htmldecode
    src/main.cpp
    src/htmldecode.h
    src/htmldecode.cpp
)

add_executable(tests
    tests/tests.cpp
    tests/main.cpp
    src/htmldecode.h
    src/htmldecode.cpp
)
target_link_libraries(tests "/home/vladimir/.conan/data/catch2/3.5.0/_/_/package/2d3baf6699d9c743852b6d73c81f1517997b5976/lib/libCatch2Main.a" "/home/vladimir/.conan/data/catch2/3.5.0/_/_/package/2d3baf6699d9c743852b6d73c81f1517997b5976/lib/libCatch2.a")
