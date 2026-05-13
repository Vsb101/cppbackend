cmake_minimum_required(VERSION 3.11)
project(postgres CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
set(CONAN_DISABLE_CHECK_COMPILER TRUE)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

file(GLOB SOURCES "*.cpp")
add_executable(book_manager ${SOURCES})
target_link_libraries(book_manager CONAN_PKG::libpqxx)
