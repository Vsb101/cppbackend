# Минимальная версия CMake
cmake_minimum_required(VERSION 3.11)

# Название проекта и язык
project(game_server CXX)

# Стандарт C++20
set(CMAKE_CXX_STANDARD 20)

# Интеграция с Conan (менеджер зависимостей)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(KEEP_RPATHS)

# Используем Boost из Conan, а не системный
# Это нужно, чтобы избежать конфликтов версий
set(Boost_NO_BOOST_CMAKE ON)
set(Boost_NO_SYSTEM_PATHS ON)
set(Boost_INCLUDE_DIR ${CONAN_INCLUDE_DIRS_BOOST})
set(Boost_LIBRARY_DIRS ${CONAN_LIB_DIRS_BOOST})

# Поиск библиотеки потоков (pthread)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Автоматически находим все .cpp и .h файлы в папке src
# Это упрощает поддержку — новые файлы добавляются автоматически
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

# Создаём исполняемый файл из всех найденных исходников
add_executable(game_server ${SOURCES})

# Пути для поиска заголовочных файлов
target_include_directories(game_server PRIVATE
    src                      # Корень исходников
    ${CONAN_INCLUDE_DIRS}    # Зависимости из Conan
)

# Библиотеки для линковки
target_link_libraries(game_server PRIVATE 
    Threads::Threads         # Потоки (pthread)
    ${CONAN_LIBS}            # Все зависимости из Conan (Boost, jsoncpp и др.)
)

# Определение макроса для использования std::string_view в Boost.Beast
add_compile_definitions(BOOST_BEAST_USE_STD_STRING_VIEW)