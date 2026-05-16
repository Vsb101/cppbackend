#include "view.h"

#include <boost/algorithm/string.hpp>
#include <cassert>
#include <iostream>
#include <map>
#include <set>
#include <sstream>

#include "../app/use_cases.h"
#include "../menu/menu.h"

using namespace std::literals;
namespace ph = std::placeholders;

namespace ui {
namespace detail {

std::ostream& operator<<(std::ostream& out, const AuthorInfo& author) {
    out << author.name;
    return out;
}

std::ostream& operator<<(std::ostream& out, const BookInfo& book) {
    out << book.title << " by " << book.author_name << ", " << book.publication_year;
    return out;
}

}  // namespace detail

template <typename T>
void PrintVector(std::ostream& out, const std::vector<T>& vector) {
    int i = 1;
    for (auto& value : vector) {
        out << i++ << " " << value << std::endl;
    }
}

View::View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output)
    : menu_{menu}
    , use_cases_{use_cases}
    , input_{input}
    , output_{output} {
    menu_.AddAction(  //
        "AddAuthor"s, "name"s, "Adds author"s, std::bind(&View::AddAuthor, this, ph::_1)
        // либо
        // [this](auto& cmd_input) { return AddAuthor(cmd_input); }
    );
    menu_.AddAction("AddBook"s, "<pub year> <title>"s, "Adds book"s,
                    std::bind(&View::AddBook, this, ph::_1));
    menu_.AddAction("ShowAuthors"s, {}, "Show authors"s, std::bind(&View::ShowAuthors, this));
    menu_.AddAction("ShowBooks"s, {}, "Show books"s, std::bind(&View::ShowBooks, this));
    menu_.AddAction("ShowBook"s, "<title> or <book#>"s, "Show book details"s,
                    std::bind(&View::ShowBook, this, ph::_1));
    menu_.AddAction("DeleteAuthor"s, "<name> or <author#>"s, "Delete author"s,
                    std::bind(&View::DeleteAuthor, this, ph::_1));
    menu_.AddAction("DeleteBook"s, "<title> or <book#>"s, "Delete book"s,
                    std::bind(&View::DeleteBook, this, ph::_1));
    menu_.AddAction("EditAuthor"s, "<name> or <author#>"s, "Edit author"s,
                    std::bind(&View::EditAuthor, this, ph::_1));
    menu_.AddAction("EditBook"s, "<title> or <book#>"s, "Edit book"s,
                    std::bind(&View::EditBook, this, ph::_1));
}

bool View::AddAuthor(std::istream& cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        boost::algorithm::trim(name);
        use_cases_.AddAuthor(std::move(name));
    } catch (const std::exception&) {
        output_ << "Failed to add author"sv << std::endl;
    }
    return true;
}

bool View::AddBook(std::istream& cmd_input) const {
    try {
        // Считываем год и название книги из команды
        int publication_year;
        cmd_input >> publication_year;
        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);

        // Спрашиваем имя автора или выбор из списка
        output_ << "Enter author name or empty line to select from list: " << std::flush;
        std::string author_input;
        std::getline(input_, author_input);
        boost::algorithm::trim(author_input);

        std::string author_id;
        bool author_added = false;

        if (author_input.empty()) {
            // Выбор автора из списка
            auto authors = GetAuthors();
            if (authors.empty()) {
                output_ << "No authors found. Failed to add book."sv << std::endl;
                return true;
            }
            PrintVector(output_, authors);
            output_ << "Enter author #" << std::flush;
            std::string author_num;
            std::getline(input_, author_num);
            boost::algorithm::trim(author_num);
            author_id = author_num;
        } else {
            // Проверка, существует ли автор
            auto authors = GetAuthors();
            auto it = std::find_if(authors.begin(), authors.end(),
                [&author_input](const auto& a) { return a.name == author_input; });
            
            if (it == authors.end()) {
                output_ << "No author found. Do you want to add " << author_input << " (y/n)? " << std::flush;
                std::string response;
                std::getline(input_, response);
                boost::algorithm::trim(response);
                if (response != "y" && response != "Y") {
                    output_ << "Failed to add book."sv << std::endl;
                    return true;
                }
                // Добавляем автора
                use_cases_.AddAuthor(author_input);
                author_id = author_input;
                author_added = true;
            } else {
                author_id = it->id;
            }
        }

        // Спрашиваем теги
        output_ << "Enter tags (comma separated): " << std::flush;
        std::string tags_input;
        std::getline(input_, tags_input);
        
        std::vector<std::string> tags;
        if (!tags_input.empty()) {
            std::istringstream ss(tags_input);
            std::string tag;
            while (std::getline(ss, tag, ',')) {
                boost::algorithm::trim(tag);
                if (!tag.empty()) {
                    tags.push_back(tag);
                }
            }
        }

        // Добавляем книгу
        auto result = use_cases_.AddBookWithAuthorSelection(title, publication_year, author_id, tags);

    } catch (const std::exception& e) {
        output_ << "Failed to add book"sv << std::endl;
    }
    return true;
}

bool View::ShowAuthors() const {
    PrintVector(output_, GetAuthors());
    return true;
}

bool View::ShowBooks() const {
    PrintVector(output_, GetBooks());
    return true;
}

bool View::ShowBook(std::istream& cmd_input) const {
    auto books = GetBooks();
    
    // Пытаемся найти книгу по названию или номеру
    std::string input;
    std::getline(cmd_input, input);
    boost::algorithm::trim(input);
    
    std::optional<detail::BookInfo> selected_book;
    
    if (input.empty()) {
        // Если ничего не введено, показываем список
        if (books.empty()) {
            return true;
        }
        PrintVector(output_, books);
        output_ << "Enter the book # or empty line to cancel: " << std::flush;
        std::string book_num;
        std::getline(input_, book_num);
        boost::algorithm::trim(book_num);
        if (book_num.empty()) {
            return true;
        }
        int book_idx = std::stoi(book_num) - 1;
        if (book_idx < 0 || book_idx >= static_cast<int>(books.size())) {
            return true;
        }
        selected_book = books[book_idx];
    } else {
        // Проверяем, является ли ввод номером
        bool is_number = true;
        for (char c : input) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                is_number = false;
                break;
            }
        }
        
        if (is_number) {
            int book_idx = std::stoi(input) - 1;
            if (book_idx < 0 || book_idx >= static_cast<int>(books.size())) {
                return true;
            }
            selected_book = books[book_idx];
        } else {
            // Поиск по названию
            auto matching_books = std::vector<detail::BookInfo>();
            for (const auto& book : books) {
                if (book.title == input) {
                    matching_books.push_back(book);
                }
            }
            
            if (matching_books.empty()) {
                return true;
            }
            
            if (matching_books.size() == 1) {
                selected_book = matching_books[0];
            } else {
                // Несколько книг с таким названием
                PrintVector(output_, matching_books);
                output_ << "Enter the book # or empty line to cancel: " << std::flush;
                std::string book_num;
                std::getline(input_, book_num);
                boost::algorithm::trim(book_num);
                if (book_num.empty()) {
                    return true;
                }
                int book_idx = std::stoi(book_num) - 1;
                if (book_idx < 0 || book_idx >= static_cast<int>(matching_books.size())) {
                    return true;
                }
                selected_book = matching_books[book_idx];
            }
        }
    }
    
    if (selected_book) {
        output_ << "Title: " << selected_book->title << std::endl;
        // Нужно получить имя автора
        auto authors = GetAuthors();
        for (const auto& book : GetBooks()) {
            if (book.title == selected_book->title && book.publication_year == selected_book->publication_year) {
                output_ << "Author: " << book.author_name << std::endl;
                output_ << "Publication year: " << book.publication_year << std::endl;
                if (!book.tags.empty()) {
                    output_ << "Tags: ";
                    for (size_t i = 0; i < book.tags.size(); ++i) {
                        if (i > 0) output_ << ", ";
                        output_ << book.tags[i];
                    }
                    output_ << std::endl;
                }
                break;
            }
        }
    }
    
    return true;
}

bool View::DeleteAuthor(std::istream& cmd_input) const {
    std::string input;
    std::getline(cmd_input, input);
    boost::algorithm::trim(input);
    
    if (input.empty()) {
        // Выбор автора из списка
        auto authors = GetAuthors();
        if (authors.empty()) {
            return true;
        }
        PrintVector(output_, authors);
        output_ << "Enter author # or empty line to cancel" << std::endl;
        std::string author_num;
        std::getline(input_, author_num);
        boost::algorithm::trim(author_num);
        if (author_num.empty()) {
            return true;
        }
        input = author_num;
    }
    
    if (!use_cases_.DeleteAuthor(input)) {
        output_ << "Failed to delete author"sv << std::endl;
    }
    
    return true;
}

bool View::DeleteBook(std::istream& cmd_input) const {
    auto books = GetBooks();
    
    std::string input;
    std::getline(cmd_input, input);
    boost::algorithm::trim(input);
    
    std::vector<detail::BookInfo> matching_books;
    
    if (input.empty()) {
        // Если ничего не введено, показываем все книги
        matching_books = books;
    } else {
        // Проверяем, является ли ввод номером
        bool is_number = true;
        for (char c : input) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                is_number = false;
                break;
            }
        }
        
        if (is_number) {
            int book_idx = std::stoi(input) - 1;
            if (book_idx >= 0 && book_idx < static_cast<int>(books.size())) {
                matching_books.push_back(books[book_idx]);
            }
        } else {
            // Поиск по названию
            for (const auto& book : books) {
                if (book.title == input) {
                    matching_books.push_back(book);
                }
            }
        }
    }
    
    if (matching_books.empty()) {
        output_ << "Failed to delete book"sv << std::endl;
        return true;
    }
    
    // Если одна книга - удаляем сразу, иначе показываем список
    int book_idx;
    if (matching_books.size() == 1 && !input.empty()) {
        book_idx = 0;
    } else {
        PrintVector(output_, matching_books);
        output_ << "Enter the book # or empty line to cancel: " << std::flush;
        std::string book_num;
        std::getline(input_, book_num);
        boost::algorithm::trim(book_num);
        if (book_num.empty()) {
            return true;
        }
        book_idx = std::stoi(book_num) - 1;
        if (book_idx < 0 || book_idx >= static_cast<int>(matching_books.size())) {
            return true;
        }
    }
    
    // Формируем ввод для DeleteBook - используем название книги
    if (!use_cases_.DeleteBook(std::to_string(book_idx + 1))) {
        output_ << "Failed to delete book"sv << std::endl;
    }
    
    return true;
}

bool View::EditAuthor(std::istream& cmd_input) const {
    auto authors = GetAuthors();
    
    std::string input;
    std::getline(cmd_input, input);
    boost::algorithm::trim(input);
    
    std::string author_input;
    
    if (input.empty()) {
        // Выбор автора из списка
        output_ << "Select author:" << std::endl;
        if (authors.empty()) {
            return true;
        }
        PrintVector(output_, authors);
        output_ << "Enter author # or empty line to cancel" << std::endl;
        std::string author_num;
        std::getline(input_, author_num);
        boost::algorithm::trim(author_num);
        if (author_num.empty()) {
            return true;
        }
        author_input = author_num;
    } else {
        author_input = input;
    }
    
    output_ << "Enter new name: " << std::flush;
    std::string new_name;
    std::getline(input_, new_name);
    boost::algorithm::trim(new_name);
    
    if (!use_cases_.EditAuthor(author_input, new_name)) {
        output_ << "Failed to edit author"sv << std::endl;
    }
    
    return true;
}

bool View::EditBook(std::istream& cmd_input) const {
    auto books = GetBooks();
    
    std::string input;
    std::getline(cmd_input, input);
    boost::algorithm::trim(input);
    
    std::optional<detail::BookInfo> selected_book;
    
    if (input.empty()) {
        // Если ничего не введено, показываем все книги
        if (books.empty()) {
            return true;
        }
        PrintVector(output_, books);
        output_ << "Enter the book # or empty line to cancel: " << std::flush;
        std::string book_num;
        std::getline(input_, book_num);
        boost::algorithm::trim(book_num);
        if (book_num.empty()) {
            return true;
        }
        int book_idx = std::stoi(book_num) - 1;
        if (book_idx < 0 || book_idx >= static_cast<int>(books.size())) {
            return true;
        }
        selected_book = books[book_idx];
    } else {
        // Проверяем, является ли ввод номером
        bool is_number = true;
        for (char c : input) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                is_number = false;
                break;
            }
        }
        
        if (is_number) {
            int book_idx = std::stoi(input) - 1;
            if (book_idx < 0 || book_idx >= static_cast<int>(books.size())) {
                output_ << "Book not found"sv << std::endl;
                return true;
            }
            selected_book = books[book_idx];
        } else {
            // Поиск по названию
            auto matching_books = std::vector<detail::BookInfo>();
            for (const auto& book : books) {
                if (book.title == input) {
                    matching_books.push_back(book);
                }
            }
            
            if (matching_books.empty()) {
                output_ << "Book not found"sv << std::endl;
                return true;
            }
            
            if (matching_books.size() == 1) {
                selected_book = matching_books[0];
            } else {
                // Несколько книг с таким названием
                PrintVector(output_, matching_books);
                output_ << "Enter the book # or empty line to cancel: " << std::flush;
                std::string book_num;
                std::getline(input_, book_num);
                boost::algorithm::trim(book_num);
                if (book_num.empty()) {
                    return true;
                }
                int book_idx = std::stoi(book_num) - 1;
                if (book_idx < 0 || book_idx >= static_cast<int>(matching_books.size())) {
                    return true;
                }
                selected_book = matching_books[book_idx];
            }
        }
    }
    
    if (!selected_book) {
        output_ << "Book not found"sv << std::endl;
        return true;
    }
    
    // Спрашиваем новые значения
    output_ << "Enter new title or empty line to use the current one (" << selected_book->title << "): " << std::flush;
    std::string new_title;
    std::getline(input_, new_title);
    boost::algorithm::trim(new_title);
    
    output_ << "Enter publication year or empty line to use the current one (" << selected_book->publication_year << "): " << std::flush;
    std::string year_input;
    std::getline(input_, year_input);
    boost::algorithm::trim(year_input);
    int new_year = 0;
    if (!year_input.empty()) {
        new_year = std::stoi(year_input);
    }
    
    // Спрашиваем теги
    std::string current_tags_str;
    if (!selected_book->tags.empty()) {
        for (size_t i = 0; i < selected_book->tags.size(); ++i) {
            if (i > 0) current_tags_str += ", ";
            current_tags_str += selected_book->tags[i];
        }
    }
    output_ << "Enter tags (current tags: " << current_tags_str << "): " << std::flush;
    std::string tags_input;
    std::getline(input_, tags_input);
    
    std::vector<std::string> new_tags;
    if (!tags_input.empty()) {
        std::istringstream ss(tags_input);
        std::string tag;
        while (std::getline(ss, tag, ',')) {
            boost::algorithm::trim(tag);
            if (!tag.empty()) {
                new_tags.push_back(tag);
            }
        }
    }
    
    // Формируем ввод для EditBook - используем номер книги
    auto books_after = GetBooks();
    int book_num = 0;
    for (size_t i = 0; i < books_after.size(); ++i) {
        if (books_after[i].title == selected_book->title && 
            books_after[i].publication_year == selected_book->publication_year) {
            book_num = static_cast<int>(i) + 1;
            break;
        }
    }
    
    if (!use_cases_.EditBook(std::to_string(book_num), new_title, new_year, new_tags)) {
        output_ << "Book not found"sv << std::endl;
    }
    
    return true;
}

std::optional<detail::AddBookParams> View::GetBookParams(std::istream& cmd_input) const {
    detail::AddBookParams params;

    cmd_input >> params.publication_year;
    std::getline(cmd_input, params.title);
    boost::algorithm::trim(params.title);

    auto author_id = SelectAuthor("Select author:");
    if (not author_id.has_value())
        return std::nullopt;
    else {
        params.author_id = author_id.value();
        return params;
    }
}

std::optional<std::string> View::SelectAuthor(const std::string& prompt) const {
    output_ << prompt << std::endl;
    auto authors = GetAuthors();
    PrintVector(output_, authors);
    output_ << "Enter author # or empty line to cancel" << std::endl;

    std::string str;
    if (!std::getline(input_, str) || str.empty()) {
        return std::nullopt;
    }

    int author_idx;
    try {
        author_idx = std::stoi(str);
    } catch (std::exception const&) {
        throw std::runtime_error("Invalid author num");
    }

    --author_idx;
    if (author_idx < 0 or author_idx >= static_cast<int>(authors.size())) {
        throw std::runtime_error("Invalid author num");
    }

    return authors[author_idx].id;
}

std::optional<std::string> View::SelectBook(const std::string& prompt, const std::vector<detail::BookInfo>& books) const {
    if (books.empty()) {
        return std::nullopt;
    }
    output_ << prompt << std::endl;
    PrintVector(output_, books);
    output_ << "Enter the book # or empty line to cancel: " << std::flush;

    std::string str;
    if (!std::getline(input_, str) || str.empty()) {
        return std::nullopt;
    }

    int book_idx;
    try {
        book_idx = std::stoi(str);
    } catch (std::exception const&) {
        throw std::runtime_error("Invalid book num");
    }

    --book_idx;
    if (book_idx < 0 or book_idx >= static_cast<int>(books.size())) {
        throw std::runtime_error("Invalid book num");
    }

    return std::to_string(book_idx + 1);
}

std::vector<detail::AuthorInfo> View::GetAuthors() const {
    std::vector<detail::AuthorInfo> dst_authors;
    try {
        auto authors = use_cases_.GetAuthors();
        for (const auto& author : authors) {
            dst_authors.push_back({author.GetId().ToString(), author.GetName()});
        }
    } catch (const std::exception&) {
        throw std::runtime_error("Failed to get authors");
    }
    return dst_authors;
}

std::vector<detail::BookInfo> View::GetBooks() const {
    std::vector<detail::BookInfo> books;
    try {
        auto all_books = use_cases_.GetBooks();
        auto authors = use_cases_.GetAuthors();
        
        // Создаем маппинг author_id -> author_name
        std::map<std::string, std::string> author_map;
        for (const auto& author : authors) {
            author_map[author.GetId().ToString()] = author.GetName();
        }
        
        for (const auto& book : all_books) {
            std::string author_name;
            auto author_it = author_map.find(book.GetAuthorId().ToString());
            if (author_it != author_map.end()) {
                author_name = author_it->second;
            }
            
            books.push_back({
                book.GetTitle(),
                author_name,
                book.GetPublicationYear(),
                book.GetTags()
            });
        }
        
        // Сортируем результат: по названию, затем по автору, затем по году
        std::sort(books.begin(), books.end(), [](const auto& a, const auto& b) {
            if (a.title != b.title) {
                return a.title < b.title;
            }
            if (a.author_name != b.author_name) {
                return a.author_name < b.author_name;
            }
            return a.publication_year < b.publication_year;
        });
    } catch (const std::exception&) {
        throw std::runtime_error("Failed to get books");
    }
    return books;
}

std::vector<detail::BookInfo> View::GetAuthorBooks(const std::string& author_id) const {
    std::vector<detail::BookInfo> books;
    try {
        auto author_books = use_cases_.GetAuthorBooks(author_id);
        auto authors = use_cases_.GetAuthors();
        
        std::string author_name;
        for (const auto& author : authors) {
            if (author.GetId().ToString() == author_id) {
                author_name = author.GetName();
                break;
            }
        }
        
        for (const auto& book : author_books) {
            books.push_back({
                book.GetTitle(),
                author_name,
                book.GetPublicationYear(),
                book.GetTags()
            });
        }
    } catch (const std::exception&) {
        throw std::runtime_error("Failed to get author books");
    }
    return books;
}

}  // namespace ui
