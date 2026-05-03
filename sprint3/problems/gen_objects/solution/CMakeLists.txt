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

# Рекурсивно ищем все исходники в src
file(GLOB_RECURSE ALL_SOURCES "src/*.cpp")

# Отделяем main.cpp от остальных файлов
set(APP_SOURCES ${ALL_SOURCES})
list(FILTER APP_SOURCES EXCLUDE REGEX "src/main.cpp$")

# --- ОСНОВНОЙ СЕРВЕР ---
add_executable(game_server src/main.cpp ${APP_SOURCES})
target_include_directories(game_server PRIVATE src)

# Линковка
target_link_libraries(game_server PRIVATE
    Threads::Threads
    CONAN_PKG::boost
)

# --- ТЕСТЫ (Нужны для сдачи задания) ---
# file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")
# if (TEST_SOURCES)
#     add_executable(game_server_tests ${TEST_SOURCES} ${APP_SOURCES})
#     target_include_directories(game_server_tests PRIVATE src)
#     target_link_libraries(game_server_tests PRIVATE
#         Threads::Threads
#         CONAN_PKG::boost
#         CONAN_PKG::catch2
#     )
# endif()

# Макросы
add_compile_definitions(
    BOOST_BEAST_USE_STD_STRING_VIEW
    BOOST_JSON_NO_LIB
)

# --- ТЕСТЫ ---
add_executable(game_server_tests
    src/model/map.h
    src/model/map.cpp
    src/model/game.h
    src/model/game.cpp
    src/model/game_session.h
    src/model/game_session.cpp
    src/model/loot_generator.h
    src/model/loot_generator.cpp
    src/model/dog.h
    src/other/tagged.h
    src/api/json_serialization.h
    src/api/json_serialization.cpp
    src/infra/json_loader.h
    src/infra/json_loader.cpp
    src/app/extra_data.h
    tests/model-tests.cpp
    tests/loot_generator_tests.cpp
    tests/json_loader_tests.cpp
)

target_include_directories(game_server_tests PRIVATE src CONAN_PKG::boost)
target_link_libraries(game_server_tests PRIVATE CONAN_PKG::catch2 CONAN_PKG::boost Threads::Threads)
