cmake_minimum_required(VERSION 3.11)
project(collision-detector-tests CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- ИНТЕГРАЦИЯ С CONAN ---
if(EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
    include("${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
elseif(EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake")
    include("${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake")
endif()

if(COMMAND conan_basic_setup)
    conan_basic_setup(TARGETS)
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Источники тестов и реализация
add_executable(collision_detector_tests
    ../src/collision_detector.cpp
    collision-detector-tests.cpp
    wrong-implementation-tests.cpp
)

target_link_libraries(collision_detector_tests PRIVATE 
    CONAN_PKG::catch2 
    CONAN_PKG::boost 
    Threads::Threads
)

target_compile_definitions(collision_detector_tests PRIVATE DEBUG_RUN)
