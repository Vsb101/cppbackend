cmake_minimum_required(VERSION 3.16)
set(Boost_NO_BOOST_CMAKE ON)
project(game_server CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- ОПЦИИ СБОРКИ ---
option(BUILD_TESTS "Build test suite" ON)
option(BUILD_WITH_ASAN "Build with AddressSanitizer" OFF)

# --- ИНТЕГРАЦИЯ С CONAN ---
if(EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
    include("${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
elseif(EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake")
    include("${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake")
elseif(EXISTS "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
    include("${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
endif()

if(COMMAND conan_basic_setup)
    conan_basic_setup(TARGETS KEEP_RPATHS)
endif()

# Отключаем проверку версии компилятора Conan (для GCC 14/15)
add_compile_definitions(CONAN_DISABLE_CHECK_COMPILER=1)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# --- ИСХОДНИКИ ---
# Явно перечисляем источники приложения (кроме main.cpp)
set(APP_SOURCES
    src/app/app.cpp
    src/infra/http_server.cpp
    src/infra/json_loader.cpp
    src/infra/state_serializer.cpp
    src/logger/logger.cpp
    src/logger/logging_handler.cpp
    src/model/collision_detector.cpp
    src/model/game.cpp
    src/model/game_session.cpp
    src/model/loot_generator.cpp
    src/model/map.cpp
    src/api/api_handler.cpp
    src/api/json_serialization.cpp
)

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

# AddressSanitizer при необходимости
if(BUILD_WITH_ASAN)
    target_compile_options(game_server PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(game_server PRIVATE -fsanitize=address)
endif()

# --- ТЕСТЫ (Catch2) ---
if(BUILD_TESTS)
    # Ищем все файлы тестов
    file(GLOB TEST_SOURCES "tests/*.cpp")

    if(TEST_SOURCES)
        add_executable(game_server_tests ${TEST_SOURCES} ${APP_SOURCES})
        target_include_directories(game_server_tests PRIVATE src)
        target_link_libraries(game_server_tests PRIVATE
            CONAN_PKG::catch2
            CONAN_PKG::boost
            CONAN_PKG::libpqxx
            Threads::Threads
        )
        target_compile_definitions(game_server_tests PRIVATE DEBUG_RUN)
        
        # Регистрация тестов в CTest
        enable_testing()
        add_test(NAME GameServerTests COMMAND game_server_tests)
    endif()
endif()

# --- УСТАНОВКА (install) ---
install(TARGETS game_server
    RUNTIME DESTINATION bin
)

if(BUILD_TESTS AND TEST_SOURCES)
    install(TARGETS game_server_tests
        RUNTIME DESTINATION bin/tests
    )
endif()

# Установка конфига и статики
install(DIRECTORY data/
    DESTINATION share/game_server/data
    FILES_MATCHING PATTERN "*.json"
)

install(DIRECTORY static/
    DESTINATION share/game_server/static
)
