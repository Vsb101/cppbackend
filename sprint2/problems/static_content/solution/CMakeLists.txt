cmake_minimum_required(VERSION 3.11)

# Отключаем поиск системного Boost
set(Boost_NO_BOOST_CMAKE ON)

project(game_server CXX)

# Стандарт C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Интеграция с Conan
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

# Поиск библиотеки потоков
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Поиск Boost с явными компонентами
find_package(Boost REQUIRED COMPONENTS
    log
    log_setup
    filesystem
    system
    thread
    chrono
    date_time
)

# Рекурсивный поиск исходников
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

# Создаём исполняемый файл
add_executable(game_server ${SOURCES})

# Пути для поиска заголовочных файлов и библиотек
target_include_directories(game_server PRIVATE
    src
    ${Boost_INCLUDE_DIR}
    ${CONAN_INCLUDE_DIRS_JSONCPP}
)

# Путь к библиотекам jsoncpp
target_link_directories(game_server PRIVATE
    ${CONAN_LIB_DIRS_JSONCPP}
)

# Библиотеки для линковки
target_link_libraries(game_server PRIVATE
    Threads::Threads
    Boost::log
    Boost::log_setup
    Boost::filesystem
    Boost::system
    Boost::thread
    Boost::chrono
    Boost::date_time
    Boost::regex
    jsoncpp
)

# Макросы
add_compile_definitions(
    BOOST_BEAST_USE_STD_STRING_VIEW
    BOOST_JSON_NO_LIB
)