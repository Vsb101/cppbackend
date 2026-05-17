#include "commands.hpp"
#include "json_parser.hpp"
#include "json_utils.hpp"

// --- AddBookCommand ---
AddBookCommand::AddBookCommand(BookRepository& repo) : repo_(repo) {}

std::string AddBookCommand::Execute(std::string_view line) {
    size_t key_pos = line.find("\"payload\"");
    if (key_pos == std::string_view::npos) {
        return "{\"result\":false}";
    }
    
    size_t start_brace = line.find('{', key_pos);
    size_t end_brace = line.rfind('}');
    if (start_brace == std::string_view::npos || end_brace == std::string_view::npos || end_brace <= start_brace) {
        return "{\"result\":false}";
    }

    std::string_view payload = line.substr(start_brace, end_brace - start_brace + 1);

    auto title_opt =    JsonParser::ExtractStringValue(payload, "title");
    auto author_opt =   JsonParser::ExtractStringValue(payload, "author");
    auto year_opt =     JsonParser::ExtractIntValue(payload, "year");
    bool is_null_isbn = JsonParser::ExtractNullValue(payload, "ISBN");
    auto isbn_opt =     JsonParser::ExtractStringValue(payload, "ISBN");

    if (!title_opt || !author_opt || !year_opt) {
        return "{\"result\":false}";
    }

    if (title_opt->length() > 100 || author_opt->length() > 100) {
        return "{\"result\":false}";
    }

    Book new_book{
        .id = 0, // Поле id по умолчанию, в БД оно сгенерируется через SERIAL
        .title = std::move(*title_opt),
        .author = std::move(*author_opt),
        .year = *year_opt,
        .isbn = (is_null_isbn || !isbn_opt || isbn_opt->empty()) ? std::nullopt : std::make_optional(std::move(*isbn_opt))
    };

    bool success = repo_.Add(new_book);
 
    return std::string("{\"result\":") + json_utils::FormatJsonBool(success) + "}";
}

// --- AllBooksCommand ---
AllBooksCommand::AllBooksCommand(BookRepository& repo) : repo_(repo) {}

std::string AllBooksCommand::Execute(std::string_view) {
    auto books = repo_.GetAll();
    
    std::string json_out = "[";
    bool first = true;
    for (const auto& book : books) {
        if (!first) {
            json_out += ",";
        }
        first = false;
        
        // Форматируем структуру книги
        json_out += std::string("{")
            + "\"id\":" + std::to_string(book.id) + ","
            + "\"title\":\"" + json_utils::EscapeJsonString(book.title) + "\","
            + "\"author\":\"" + json_utils::EscapeJsonString(book.author) + "\","
            + "\"year\":" + std::to_string(book.year) + ","
            + "\"ISBN\":" + json_utils::FormatNullableString(book.isbn)
            + "}";
    }
    json_out += "]";
    return json_out;
}
