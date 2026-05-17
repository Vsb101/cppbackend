cmake_minimum_required(VERSION 3.11)

project(bookypedia CXX)
set(CMAKE_CXX_STANDARD 20)

# Conan paths
include(${CMAKE_BINARY_DIR}/conan_paths.cmake OPTIONAL)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(Boost REQUIRED)
find_package(libpqxx REQUIRED)

# Автоматический поиск исходных файлов
file(GLOB_RECURSE LIB_SOURCES
	CONFIGURE_DEPENDS
	src/*.cpp
	src/*.h
)

add_library(libbookypedia STATIC ${LIB_SOURCES})
target_link_libraries(libbookypedia PUBLIC Boost::headers Threads::Threads libpqxx::libpqxx)

# Автоматический поиск файлов приложения
file(GLOB APP_SOURCES
	src/main.cpp
	src/bookypedia.cpp
	src/bookypedia.h
)

add_executable(bookypedia ${APP_SOURCES})
target_link_libraries(bookypedia PRIVATE Boost::headers libbookypedia)

# Автоматический поиск файлов тестов
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS tests/*.cpp)

add_executable(tests ${TEST_SOURCES})
find_package(Catch2 REQUIRED)
find_package(GTest REQUIRED)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain GTest::gtest libbookypedia)
