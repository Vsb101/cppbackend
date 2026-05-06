cmake_minimum_required(VERSION 3.11)
project(collision_detector_tests CXX)

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

# --- ТЕСТЫ (Catch2) ---
add_executable(collision_detection_tests
    src/collision_detector.cpp
    tests/collision-detector-tests.cpp
)

target_link_libraries(collision_detection_tests PRIVATE 
    CONAN_PKG::catch2 
    CONAN_PKG::boost 
    Threads::Threads
)

target_compile_definitions(collision_detection_tests PRIVATE DEBUG_RUN)
