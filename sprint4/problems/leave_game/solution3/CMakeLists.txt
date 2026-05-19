cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

# обратите внимание на аргумент TARGETS у команды conan_basic_setup
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Добавляем библиотеку моедли, указывая, что она статическая.
add_library(model STATIC 
	src/model/buildings.cpp
	src/model/buildings.h
	src/model/dog.cpp
	src/model/dog.h
	src/model/game_session.cpp
	src/model/game_session.h
	src/model/game.cpp
	src/model/game.h
	src/model/item.cpp
	src/model/item.h
	src/model/map.cpp
	src/model/map.h
	src/model/model_geom.cpp
	src/model/model_geom.h
	src/model/model_serialization.h
	src/model/road.cpp
	src/model/road.h
	src/model/utils.cpp
	src/model/utils.h
	src/utils/collision_detector.cpp
	src/utils/collision_detector.h
	src/utils/loot_generator.cpp
	src/utils/loot_generator.h
	src/utils/tagged.h
	src/utils/write_adapter.h
)

target_include_directories(model PUBLIC src/model src/utils)

add_library(app STATIC 
	src/app/add_player_use_case.cpp
	src/app/add_player_use_case.h
	src/app/app_serialization.h
	src/app/app.cpp
	src/app/app.h
	src/app/join_use_case.cpp
	src/app/join_use_case.h
	src/app/maps_use_case.cpp
	src/app/maps_use_case.h
	src/app/player_use_case.cpp
	src/app/player_use_case.h
	src/app/players_fwd.h
	src/app/players_use_case.cpp
	src/app/players_use_case.h
	src/app/players.cpp
	src/app/players.h
	src/app/records_use_case.cpp
	src/app/records_use_case.h
	src/app/state_use_case.cpp
	src/app/state_use_case.h
	src/app/tick_use_case.cpp
	src/app/tick_use_case.h
	src/app/use_cases_impl.cpp
	src/app/use_cases_impl.h
	src/app/use_cases.h
	src/extra_data/extra_data.cpp
	src/extra_data/extra_data.h
	src/postgres/postgres.cpp
	src/postgres/postgres.h
	src/utils/tagged_uuid.cpp
	src/utils/tagged_uuid.h
)

target_include_directories(app PUBLIC src src/app src/model src/extra_data src/utils CONAN_PKG::boost CONAN_PKG::libpq CONAN_PKG::libpqxx)
target_link_libraries(app PUBLIC CONAN_PKG::boost CONAN_PKG::libpq CONAN_PKG::libpqxx)

add_library(http STATIC 
	src/http/api_handler.cpp
	src/http/api_handler.h
	src/http/file_handler.cpp
	src/http/file_handler.h
	src/http/http_handler_defs.h
	src/http/http_handler_types.cpp
	src/http/http_handler_types.h
	src/http/http_server.cpp
	src/http/http_server.h
	src/http/request_handler_logging.h
	src/http/request_handler.cpp
	src/http/request_handler.h
	src/server_params.h
	src/sdk.h
)

target_include_directories(http PUBLIC src src/app src/http src/model src/utils src/extra_data CONAN_PKG::boost CONAN_PKG::libpq CONAN_PKG::libpqxx)
target_link_libraries(http PUBLIC CONAN_PKG::boost CONAN_PKG::libpq CONAN_PKG::libpqxx)

add_library(utils STATIC 
	src/utils/boost_json.cpp
	src/utils/geom.h
	src/utils/json_fields.h
	src/utils/json_loader.cpp
	src/utils/json_loader.h
	src/utils/logger.cpp
	src/utils/logger.h
	src/utils/ticker.h
)

target_include_directories(utils PUBLIC src src/model src/utils src/http src/app src/extra_data CONAN_PKG::boost CONAN_PKG::libpq CONAN_PKG::libpqxx)
target_link_libraries(utils PUBLIC CONAN_PKG::boost CONAN_PKG::libpq CONAN_PKG::libpqxx)

add_executable(game_server
	src/main.cpp
)

# используем "импортированную" цель CONAN_PKG::boost
# target_include_directories(game_server PRIVATE CONAN_PKG::boost)
target_link_libraries(game_server app http utils model Threads::Threads)


include(CTest)
include(Catch)
include(${CONAN_BUILD_DIRS_CATCH2}/Catch.cmake)

add_executable(game_server_tests
	tests/tests_main.cpp
	tests/collision-detector-tests.cpp
	tests/loot_generator_tests.cpp
	tests/model_tests.cpp
)

# target_include_directories(game_server_tests PRIVATE src/utils)
target_link_libraries(game_server_tests model CONAN_PKG::catch2 utils)

catch_discover_tests(game_server_tests)