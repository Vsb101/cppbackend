#include <iostream>
#include <string>
#include <optional>
#include <vector>
#include <sstream>
#include <iomanip>
#include <pqxx/pqxx>

#include "json_parser.hpp"
#include "json_utils.hpp"

using namespace std::literals;
using pqxx::operator""_zv;

int main(int argc, const char* argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: book_manager <conn-string>\n"sv;
            return EXIT_FAILURE;
        }

        pqxx::connection conn{argv[1]};

        // Создаём таблицу и индекс сразу при старте через обычный exec
        {
            pqxx::work w(conn);
            w.exec(
                "CREATE TABLE IF NOT EXISTS books (id SERIAL PRIMARY KEY, title varchar(100) NOT NULL, "
                "author varchar(100) NOT NULL, year integer NOT NULL, ISBN char(13));"_zv);
            w.exec(
                "CREATE UNIQUE INDEX IF NOT EXISTS idx_books_isbn ON books (ISBN) WHERE ISBN IS NOT NULL;"_zv);
            w.commit();
        }

        // Подготавливаем запросы для данных
        conn.prepare("add_book",
            "INSERT INTO books (title, author, year, ISBN) VALUES ($1, $2, $3, $4);"_zv);
        
        conn.prepare("add_book_null_isbn",
            "INSERT INTO books (title, author, year, ISBN) VALUES ($1, $2, $3, NULL);"_zv);
        
        conn.prepare("all_books",
            "SELECT id, title, author, year, ISBN FROM books ORDER BY year DESC, title ASC, author ASC, ISBN ASC;"_zv);

        std::string line;
        while (std::getline(std::cin, line)) {
            if (line.empty()) continue;

            try {
                auto action_opt = JsonParser::ExtractStringValue(line, "action");
                if (!action_opt) {
                    std::cout << "{\"result\":false}\n"sv;
                    continue;
                }
                std::string action = *action_opt;

                if (action == "add_book") {
                    // Извлекаем данные из payload
                    size_t payload_start = line.find("\"payload\"");
                    if (payload_start == std::string::npos) {
                        std::cout << "{\"result\":false}\n"sv;
                        continue;
                    }

                    std::string payload = line.substr(payload_start);
                    
                    auto title_opt = JsonParser::ExtractStringValue(payload, "title");
                    auto author_opt = JsonParser::ExtractStringValue(payload, "author");
                    auto year_opt = JsonParser::ExtractIntValue(payload, "year");
                    bool is_null_isbn = JsonParser::ExtractNullValue(payload, "ISBN");
                    auto isbn_opt = JsonParser::ExtractStringValue(payload, "ISBN");

                    if (!title_opt || !author_opt || !year_opt) {
                        std::cout << "{\"result\":false}\n"sv;
                        continue;
                    }

                    bool success = false;
                    try {
                        pqxx::work w(conn);
                        if (is_null_isbn || !isbn_opt) {
                            w.exec_prepared("add_book_null_isbn", *title_opt, *author_opt, *year_opt);
                        } else {
                            w.exec_prepared("add_book", *title_opt, *author_opt, *year_opt, *isbn_opt);
                        }
                        w.commit();
                        success = true;
                    } catch (const pqxx::sql_error&) {
                        // Ошибка уникальности ISBN или другая ошибка SQL
                        success = false;
                    }

                    std::cout << "{\"result\":" << JsonUtils::FormatJsonBool(success) << "}\n"sv;

                } else if (action == "all_books") {
                    pqxx::read_transaction r(conn);
                    
                    std::vector<std::string> books;
                    for (auto [id, title, author, year, isbn] : 
                         r.query<int, std::string, std::string, int, std::optional<std::string>>(
                             "SELECT id, title, author, year, ISBN FROM books ORDER BY year DESC, title ASC, author ASC, ISBN ASC;"_zv)) {
                        
                        std::ostringstream book_json;
                        book_json << "{\"id\":" << id
                                 << ",\"title\":\"" << JsonUtils::EscapeJsonString(title) << "\""
                                 << ",\"author\":\"" << JsonUtils::EscapeJsonString(author) << "\""
                                 << ",\"year\":" << year
                                 << ",\"ISBN\":" << JsonUtils::FormatNullableString(isbn) << "}";
                        books.push_back(book_json.str());
                    }

                    std::cout << "[";
                    for (size_t i = 0; i < books.size(); ++i) {
                        if (i > 0) std::cout << ",";
                        std::cout << books[i];
                    }
                    std::cout << "]\n"sv;

                } else if (action == "exit") {
                    break;
                }

            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
            }
        }

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
