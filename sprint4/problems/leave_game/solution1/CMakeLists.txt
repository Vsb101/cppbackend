cmake_minimum_required(VERSION 3.11)
set(Boost_NO_BOOST_CMAKE ON)
project(game_server CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CONAN_DISABLE_CHECK_COMPILER ON)

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

# Исключаем main.cpp, чтобы он не конфликтовал с main в тестах
set(APP_SOURCES ${ALL_SOURCES})
list(FILTER APP_SOURCES EXCLUDE REGEX "src/main.cpp$")

# Макросы
add_compile_definitions(
    BOOST_BEAST_USE_STD_STRING_VIEW
    BOOST_JSON_NO_LIB
)

# --- ОСНОВНОЙ СЕРВЕР ---
add_executable(game_server src/main.cpp ${APP_SOURCES})
target_include_directories(game_server PRIVATE src)
target_link_libraries(game_server PRIVATE
    Threads::Threads
    CONAN_PKG::boost
    CONAN_PKG::libpqxx
)

# --- ТЕСТЫ (Catch2) ---
# Ищем все файлы тестов
file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")

if(TEST_SOURCES)
    # Добавляем тесты, если есть исходники
    add_executable(game_server_tests 
        ${TEST_SOURCES} 
        ${APP_SOURCES}  # Добавляем все исходники приложения автоматически
    )

    target_include_directories(game_server_tests PRIVATE src)
    target_link_libraries(game_server_tests PRIVATE 
        CONAN_PKG::catch2
        CONAN_PKG::boost
        CONAN_PKG::libpqxx
        Threads::Threads
    )
    
    # Чтобы тесты понимали, где искать файлы данных (опционально)
    target_compile_definitions(game_server_tests PRIVATE DEBUG_RUN)
endif()
