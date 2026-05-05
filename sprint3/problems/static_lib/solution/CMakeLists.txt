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

# Макросы
add_compile_definitions(
    BOOST_BEAST_USE_STD_STRING_VIEW
    BOOST_JSON_NO_LIB
)

# --- СТАТИЧЕСКАЯ БИБЛИОТЕКА С МОДЕЛЬЮ ---
# Общая библиотека для программы и тестов
set(MODEL_SOURCES
    src/model/map.cpp
    src/model/game.cpp
    src/model/game_session.cpp
    src/model/loot_generator.cpp
)

add_library(game_model STATIC ${MODEL_SOURCES})
target_include_directories(game_model PUBLIC src)
target_link_libraries(game_model PUBLIC
    CONAN_PKG::boost
    Threads::Threads
)

# --- ОСНОВНОЙ СЕРВЕР ---
# Ищем все исходники приложения кроме модели и main.cpp
file(GLOB_RECURSE ALL_SOURCES "src/*.cpp")

set(APP_SOURCES ${ALL_SOURCES})
list(FILTER APP_SOURCES EXCLUDE REGEX "src/main.cpp$")
list(FILTER APP_SOURCES EXCLUDE REGEX "src/model/")

add_executable(game_server src/main.cpp ${APP_SOURCES})
target_link_libraries(game_server PRIVATE
    game_model
    Threads::Threads
    CONAN_PKG::boost
)

# --- ТЕСТЫ (Catch2) ---
# Ищем все файлы тестов
file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")

if(TEST_SOURCES)
    # Ищем дополнительные источники для тестов (кроме модели)
    set(TEST_APP_SOURCES ${ALL_SOURCES})
    list(FILTER TEST_APP_SOURCES EXCLUDE REGEX "src/main.cpp$")
    list(FILTER TEST_APP_SOURCES EXCLUDE REGEX "src/model/")
    
    add_executable(game_server_tests 
        ${TEST_SOURCES} 
        ${TEST_APP_SOURCES}
    )

    target_link_libraries(game_server_tests PRIVATE 
        game_model
        CONAN_PKG::catch2 
        CONAN_PKG::boost 
        Threads::Threads
    )
    
    # Чтобы тесты понимали, где искать файлы данных (опционально)
    target_compile_definitions(game_server_tests PRIVATE DEBUG_RUN)
endif()
