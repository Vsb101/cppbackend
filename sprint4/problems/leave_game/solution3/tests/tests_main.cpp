#define CONFIG_CATCH_MAIN
#include <catch2/catch_session.hpp>
#include <iostream>

int test_main(int argc, char* argv[]) {
    std::cout << "Downloading files for test..." << std::endl;
    return Catch::Session().run(argc, argv);
}