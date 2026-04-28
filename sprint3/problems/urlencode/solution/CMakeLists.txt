cmake_minimum_required(VERSION 3.11)

project(urlencode CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

include_directories(/home/vladimir/.conan/data/gtest/1.14.0/_/_/package/f9f33c20295ef67fb22d4a28ff92def5bcd4bde5/include)

add_executable(urlencode
    src/main.cpp
    src/urlencode.h
    src/urlencode.cpp
)

add_executable(tests
    tests/tests.cpp
    src/urlencode.h
    src/urlencode.cpp
)
# Явно линкуем библиотеки
target_link_libraries(tests PRIVATE
    /home/vladimir/.conan/data/gtest/1.14.0/_/_/package/f9f33c20295ef67fb22d4a28ff92def5bcd4bde5/lib/libgtest_main.a
    /home/vladimir/.conan/data/gtest/1.14.0/_/_/package/f9f33c20295ef67fb22d4a28ff92def5bcd4bde5/lib/libgtest.a
    /home/vladimir/.conan/data/gtest/1.14.0/_/_/package/f9f33c20295ef67fb22d4a28ff92def5bcd4bde5/lib/libgmock.a
    pthread
)