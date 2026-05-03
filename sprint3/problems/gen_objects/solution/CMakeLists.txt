cmake_minimum_required(VERSION 3.11)
set(Boost_NO_BOOST_CMAKE ON)
project(game_server CXX)

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

# !!! УДАЛИ ИЛИ ЗАКОММЕНТИРУЙ ЭТОТ БЛОК, ОН ВЫЗЫВАЕТ ОШИБКУ !!!
# find_package(Boost REQUIRED COMPONENTS ...)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")
set(APP_SOURCES ${SOURCES})
list(FILTER APP_SOURCES EXCLUDE REGEX "src/main.cpp$")

add_executable(game_server src/main.cpp ${APP_SOURCES})
target_include_directories(game_server PRIVATE src)

# Линкуем через Конан (он уже знает про все компоненты Boost)
target_link_libraries(game_server PRIVATE
    Threads::Threads
    CONAN_PKG::boost
)

add_compile_definitions(
    BOOST_BEAST_USE_STD_STRING_VIEW
    BOOST_JSON_NO_LIB
)
