cmake_minimum_required(VERSION 3.11)

project(urldecode CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

add_executable(urldecode
    src/main.cpp
    src/urldecode.h
    src/urldecode.cpp
)

add_executable(tests
    tests/tests.cpp
    src/urldecode.h
    src/urldecode.cpp
)
target_link_libraries(tests PRIVATE CONAN_PKG::boost)

# Примечание: Conan 1 с cmake_multi не передаёт пути к заголовкам через TARGETS.
# Путь к Boost нужно указывать явно. 
# Дальше конан2?
target_include_directories(tests SYSTEM PRIVATE 
    /home/vladimir/.conan/data/boost/1.78.0/_/_/package/2fcdaae676f7e0d206944cbe93c1ba6fd5208320/include)

