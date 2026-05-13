#include <iostream>
#include <pqxx/pqxx>
#include "app.hpp"

using namespace std::literals;

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: book_manager <conn-string>\n"sv;
        return EXIT_FAILURE;
    }

    try {
        BookManagerApp app(argv[1]);
        app.Run();
    } catch (const pqxx::broken_connection& e) {
        std::cerr << "Database connection failed: " << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (const std::exception& e) {
        std::cerr << "Fatal application error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << std::flush;
    return EXIT_SUCCESS;
}
