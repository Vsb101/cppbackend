cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

find_package(Boost 1.78.0 REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include_directories(src)

add_executable(game_server
	src/main.cpp
	src/http_server.cpp
	src/http_server.h
	src/sdk.h
	src/tagged.h
	src/boost_json.cpp
	src/logger.h
	src/logger.cpp
	src/logging_request_handler.h
	src/json_builder.h
	src/json_builder.cpp
	src/json_loader.h
	src/json_loader.cpp
	src/model/model.h
	src/model/model.cpp
	src/app/application.h
	src/app/application.cpp
	src/app/player.h
	src/app/player_tokens.h
	src/app/use_cases/join_game_use_case.h
	src/app/use_cases/join_game_use_case.cpp
	src/http_handler/api_handler.h
	src/http_handler/api_handler.cpp
	src/http_handler/request_handler.h
	src/http_handler/request_handler.cpp
)
target_link_libraries(game_server PRIVATE Threads::Threads)
