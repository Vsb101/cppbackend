cmake_minimum_required(VERSION 3.11)

project(Radio CXX)
set(CMAKE_CXX_STANDARD 20)

# Подключаем conan
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

# Ищем Boost (нужен только для include, если не передаётся через Conan)
find_package(Boost 1.78.0 REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Определяем NOMINMAX только под Windows
if(WIN32)
    add_compile_definitions(NOMINMAX)
endif()

# Создаём исполняемый файл (только .cpp!)
add_executable(radio src/main.cpp)

# Привязываем библиотеки через Conan TARGETS (важно!)
target_link_libraries(radio PRIVATE
    CONAN_PKG::boost
    CONAN_PKG::miniaudio
    Threads::Threads
)