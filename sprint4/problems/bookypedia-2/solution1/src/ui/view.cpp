#include "view.h"

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string.hpp>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <chrono>

#include "../app/use_cases.h"
#include "../menu/menu.h"

using namespace std::literals;
namespace ph = std::placeholders;

namespace ui {

// Функции (без static, чтобы линкер их видел)
bool IsNumber(std::string_view str) {
    if (str.empty()) return false;
    return std::all_of(str.begin(), str.end(), 
                      [](unsigned char c) { return std::isdigit(c); });
}

std::optional<int> ParseIndex(std::string_view str) {
    if (str.empty()) return std::nullopt;
    
    size_t start = str.find_first_not_of(" \t");
    if (start == std::string_view::npos) return std::nullopt;
    str = str.substr(start);
    
    int result;
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), result);
    if (ec == std::errc{} && ptr == str.data() + str.size()) {
        return result - 1;
    }
    return std::nullopt;
}

std::vector<std::string> ParseTags(std::string_view input) {
    std::vector<std::string> tags;
    if (input.empty()) return tags;
    
    std::string str(input);
    
    while (str.find("  ") != std::string::npos) {
        boost::algorithm::replace_all(str, "  ", " ");
    }
    boost::algorithm::replace_all(str, ", ", ",");
    boost::algorithm::replace_all(str, " ,", ",");
    
    std::istringstream ss{str};
    for (std::string tag; std::getline(ss, tag, ','); ) {
        boost::algorithm::trim(tag);
        if (!tag.empty()) {
            tags.push_back(std::move(tag));
        }
    }
    
    std::sort(tags.begin(), tags.end());
    tags.erase(std::unique(tags.begin(), tags.end()), tags.end());
    return tags;
}

namespace detail {

std::ostream& operator<<(std::ostream& out, const AuthorInfo& author) {
    return out << author.name;
}

std::ostream& operator<<(std::ostream& out, const BookInfo& book) {
    return out << book.title << " by " << book.author_name << ", " << book.publication_year;
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
    menu_.AddAction("AddAuthor"s, "name"s, "Adds author"s, [this](std::istream& is) { return AddAuthor(is); });
    menu_.AddAction("DeleteAuthor"s, "name"s, "Delete author"s, [this](std::istream& is) { return DeleteAuthor(is); });
    menu_.AddAction("EditAuthor"s, "name"s, "Edit author"s, [this](std::istream& is) { return EditAuthor(is); });
    menu_.AddAction("AddBook"s, "<pub year> <title>"s, "Adds book"s, [this](std::istream& is) { return AddBook(is); });
    menu_.AddAction("ShowAuthors"s, {}, "Show authors"s, [this](std::istream&) { return ShowAuthors(); });
    menu_.AddAction("ShowBooks"s, {}, "Show books"s, [this](std::istream&) { return ShowBooks(); });
    menu_.AddAction("ShowBook"s, "title"s, "Show book"s, [this](std::istream& is) { return ShowBook(is); });
    menu_.AddAction("DeleteBook"s, "title"s, "Delete book"s, [this](std::istream& is) { return DeleteBook(is); });
    menu_.AddAction("EditBook"s, "title"s, "Edit book"s, [this](std::istream& is) { return EditBook(is); });
    menu_.AddAction("ShowAuthorBooks"s, {}, "Show author books"s, [this](std::istream&) { return ShowAuthorBooks(); });
}

void View::InvalidateCache() const {
    cached_authors_.reset();
    cached_books_.reset();
    last_cache_update_ = std::chrono::steady_clock::now();
}

const std::vector<detail::AuthorInfo>& View::GetCachedAuthors() const {
    if (!cache_enabled_) {
        cached_authors_ = GetAuthors();
        return *cached_authors_;
    }
    
    constexpr auto CACHE_TTL = std::chrono::milliseconds(100);
    
    auto now = std::chrono::steady_clock::now();
    if (!cached_authors_ || (now - last_cache_update_) > CACHE_TTL) {
        cached_authors_ = GetAuthors();
        last_cache_update_ = now;
    }
    return *cached_authors_;
}

const std::vector<detail::BookInfo>& View::GetCachedBooks() const {
    if (!cache_enabled_) {
        cached_books_ = GetBooks();
        return *cached_books_;
    }
    
    constexpr auto CACHE_TTL = std::chrono::milliseconds(100);
    
    auto now = std::chrono::steady_clock::now();
    if (!cached_books_ || (now - last_cache_update_) > CACHE_TTL) {
        cached_books_ = GetBooks();
        last_cache_update_ = now;
    }
    return *cached_books_;
}

bool View::AddAuthor(std::istream& cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        boost::algorithm::trim(name);
        if (name.empty()) {
            output_ << "Failed to add author"sv << std::endl;
            return true;
        }
        use_cases_.AddAuthor(std::move(name));
        InvalidateCache();
    } catch (const std::exception&) {
        output_ << "Failed to add author"sv << std::endl;
    }
    return true;
}

std::string View::ParsingNameAuthorOrSelect(std::istream& cmd_input) const {
    std::string name;
    std::getline(cmd_input, name);
    boost::algorithm::trim(name);
    std::string id;
    if (name.empty()) {
        auto author = SelectAuthor();
        if (author) {
            id = author->id;
        }
    } else {
        // Ищем автора по имени через GetAuthors
        auto authors = GetCachedAuthors();
        auto it = std::find_if(authors.begin(), authors.end(), 
            [&name](const auto& a) { return a.name == name; });
        if (it != authors.end()) {
            id = it->id;
        } else {
            throw std::logic_error("");
        }
    }
    return id;
}

std::string View::ParsingTitleBookOrSelect(std::istream& cmd_input) const {
    std::string title;
    std::getline(cmd_input, title);
    boost::algorithm::trim(title);
    std::string book_id;
    if (title.empty()) {
        book_id = SelectBook(GetCachedBooks(), false);
    } else {
        // Ищем книги по названию
        auto books = GetCachedBooks();
        std::vector<detail::BookInfo> matching;
        for (const auto& book : books) {
            if (book.title == title) {
                matching.push_back(book);
            }
        }
        book_id = SelectBook(matching, true);
    }
    return book_id;
}

bool View::DeleteAuthor(std::istream& cmd_input) const {
    try {
        std::string author_id = ParsingNameAuthorOrSelect(cmd_input);
        if (author_id.empty()) {
            return true;
        }
        // Передаем ID или имя
        use_cases_.DeleteAuthor(author_id);
        InvalidateCache();
    } catch (const std::exception&) {
        output_ << "Failed to delete author"sv << std::endl;
    }
    return true;
}

bool View::EditAuthor(std::istream& cmd_input) const {
    try {
        std::string author_id = ParsingNameAuthorOrSelect(cmd_input);
        if (author_id.empty()) {
            return true;
        }
        output_ << "Enter new name:" << std::endl;
        std::string new_name;
        if (!std::getline(input_, new_name) || new_name.empty()) {
            return true;
        }
        use_cases_.EditAuthor(author_id, new_name);
        InvalidateCache();
    } catch (const std::exception&) {
        output_ << "Failed to edit author"sv << std::endl;
    }
    return true;
}

bool View::AddBook(std::istream& cmd_input) const {
    try {
        int publication_year;
        if (!(cmd_input >> publication_year)) {
            output_ << "Failed to add book"sv << std::endl;
            cmd_input.clear();
            std::string dummy;
            std::getline(cmd_input, dummy);
            return true;
        }
        
        std::string title;
        std::getline(cmd_input >> std::ws, title);
        boost::algorithm::trim(title);

        if (title.empty()) {
            output_ << "Failed to add book"sv << std::endl;
            return true;
        }

        output_ << "Enter author name or empty line to select from list:" << std::endl;
        
        std::string author_input;
        std::getline(input_, author_input);
        boost::algorithm::trim(author_input);

        std::string final_author_input = author_input;
        bool cancelled = false;

        if (author_input.empty()) {
            auto authors = GetCachedAuthors();
            if (authors.empty()) {
                output_ << "Failed to add book"sv << std::endl;
                return true;
            }
            output_ << "Select author:"sv << std::endl;
            PrintVector(output_, authors);
            output_ << "Enter author # or empty line to cancel" << std::endl;
            
            std::string number_input;
            std::getline(input_, number_input);
            boost::algorithm::trim(number_input);
            
            if (number_input.empty()) {
                output_ << "Failed to add book"sv << std::endl;
                return true;
            }
            
            auto idx = ParseIndex(number_input);
            if (!idx || *idx < 0 || *idx >= static_cast<int>(authors.size())) {
                output_ << "Failed to add book"sv << std::endl;
                return true;
            }
            final_author_input = authors[*idx].name;
        } else {
            auto authors = GetCachedAuthors();
            auto it = std::find_if(authors.begin(), authors.end(), 
                [&author_input](const auto& a) { return a.name == author_input; });
            
            if (it == authors.end()) {
                output_ << "No author found. Do you want to add " << author_input << " (y/n)?" << std::endl;
                
                std::string response;
                std::getline(input_, response);
                boost::algorithm::trim(response);
                if (response != "y" && response != "Y") {
                    output_ << "Failed to add book"sv << std::endl;
                    return true;
                }
            }
        }

        output_ << "Enter tags (comma separated):" << std::endl;
        std::string tags_input;
        std::getline(input_, tags_input);
        
        use_cases_.AddBookWithAuthorSelection(title, publication_year, final_author_input, ParseTags(tags_input));
        InvalidateCache();

    } catch (const std::exception&) {
        output_ << "Failed to add book"sv << std::endl;
    }
    return true;
}

bool View::ShowAuthors() const {
    PrintVector(output_, GetCachedAuthors());
    return true;
}

void PrintBooks(std::ostream& out, const std::vector<detail::BookInfo>& books) {
    int i = 1;
    for (auto& book : books) {
        out << i++ << " " << book.title 
        << " by " << book.author_name
        << ", " << book.publication_year
        << std::endl;
    }
}

void PrintTags(std::ostream& out, const std::vector<std::string>& tags){
    bool first_tag = true;
    for (const auto& tag : tags) {
        if (first_tag) {
            first_tag = false;
        } else {
            out << ", "sv;
        }
        out << tag;
    }
}

void PrintBook(std::ostream& out, const detail::BookInfo& book) {
    out << "Title: " << book.title << "\n"
    << "Author: " << book.author_name << "\n"
    << "Publication year: " << book.publication_year  << std::endl;
    if (!book.tags.empty()) {
        out << "Tags: ";
        PrintTags(out, book.tags); 
        out << std::endl;
    }
}

bool View::ShowBooks() const {
    PrintBooks(output_, GetCachedBooks());
    return true;
}

bool View::ShowBook(std::istream& cmd_input) const {
    try {
        std::string book_id = ParsingTitleBookOrSelect(cmd_input);
        if (book_id.empty()) {
            return true;
        }
        // Получаем книгу по ID
        auto books = GetCachedBooks();
        auto it = std::find_if(books.begin(), books.end(), 
            [&book_id](const auto& b) { return b.id == book_id; });
        if (it != books.end()) {
            PrintBook(output_, *it);
        }
    } catch (const std::exception&) {
    }
    return true;
}

bool View::DeleteBook(std::istream& cmd_input) const {
    try {
        std::string book_id = ParsingTitleBookOrSelect(cmd_input);
        if (book_id.empty()) {
            return true;
        }
        use_cases_.DeleteBook(book_id);
        InvalidateCache();
    } catch (const std::exception&) {
    }
    return true;
}

bool View::EditBook(std::istream& cmd_input) const {
    try {
        std::string book_id = ParsingTitleBookOrSelect(cmd_input);
        if (book_id.empty()) {
            return true;
        }
        auto book = EditBookInterface(book_id);
        if (book) {
            use_cases_.EditBook(book->id, book->title, book->publication_year, book->tags);
            InvalidateCache();
        }
    } catch (const std::exception& e) {
        output_ << e.what() << std::endl;
    }
    return true;
}

bool View::ShowAuthorBooks() const {
    try {
        if (auto author_id = SelectAuthor()) {
            auto books = GetCachedBooks();
            std::vector<detail::BookInfo> author_books;
            for (const auto& book : books) {
                if (book.author_name == author_id->name) {
                    author_books.push_back(book);
                }
            }
            PrintVector(output_, author_books);
        }
    } catch (const std::exception&) {
        throw std::runtime_error("Failed to Show Books");
    }
    return true;
}

std::optional<detail::AuthorInfo> View::SelectAuthor() const {
    output_ << "Select author:" << std::endl;
    auto authors = GetCachedAuthors();
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
    if (author_idx < 0 or author_idx >= authors.size()) {
        throw std::runtime_error("Invalid author num");
    }

    return authors[author_idx];
}

std::string View::SelectBook(const std::vector<detail::BookInfo>& books, bool auto_select_one_book) const {
    if (books.empty()) {
        throw std::logic_error("Book not found");
    } else if (books.size() == 1 && auto_select_one_book) {
        return books.at(0).id;
    }

    PrintBooks(output_, books);
    output_ << "Enter the book # or empty line to cancel:" << std::endl;

    std::string str;
    if (!std::getline(input_, str) || str.empty()) {
        throw std::logic_error("Book not found");
    }

    int book_idx;
    try {
        book_idx = std::stoi(str);
    } catch (std::exception const&) {
        throw std::runtime_error("Invalid book num");
    }

    --book_idx;
    if (book_idx < 0 or book_idx >= books.size()) {
        throw std::runtime_error("Invalid book num");
    }

    return books[book_idx].id;
}

std::optional<detail::AuthorInfo> View::EnterAuthor() const {
    output_ << "Enter author name or empty line to select from list:" << std::endl;
    std::string name;
    if (!std::getline(input_, name) || name.empty()) {
        return std::nullopt;
    }
    
    // Ищем автора по имени
    auto authors = GetCachedAuthors();
    auto it = std::find_if(authors.begin(), authors.end(),
        [&name](const auto& a) { return a.name == name; });
    if (it != authors.end()) {
        return *it;
    }

    std::string str;
    output_ << "No author found. Do you want to add " + name + " (y/n)?" << std::endl;
    if (!std::getline(input_, str) || str.empty() || (str != "Y" && str != "y")) {
        throw std::logic_error("The user refused to enter or select the author's name.");
    }
    return {{.id = "", .name = name}};
}

std::vector<std::string> View::EnterBookTags() const {
    output_ << "Enter tags (comma separated):" << std::endl;
    std::string str;
    std::getline(input_, str);
    return ParseTags(str);
}

std::vector<detail::AuthorInfo> View::GetAuthors() const {
    auto authors = use_cases_.GetAuthors();
    std::vector<detail::AuthorInfo> result;
    result.reserve(authors.size());
    for (const auto& author : authors) {
        result.push_back({author.GetId().ToString(), author.GetName()});
    }
    return result;
}

std::vector<detail::BookInfo> View::GetBooks() const {
    auto books = use_cases_.GetBooks();
    std::vector<detail::BookInfo> result;
    result.reserve(books.size());
    
    // Получаем всех авторов для маппинга
    auto authors = use_cases_.GetAuthors();
    std::unordered_map<std::string, std::string> author_map;
    for (const auto& author : authors) {
        author_map[author.GetId().ToString()] = author.GetName();
    }
    
    for (const auto& book : books) {
        std::string author_name = "Unknown";
        auto it = author_map.find(book.GetAuthorId().ToString());
        if (it != author_map.end()) {
            author_name = it->second;
        }
        
        // Получаем теги книги
        auto tags = use_cases_.GetBookTags(book.GetId().ToString());
        
        result.push_back({
            book.GetId().ToString(),
            book.GetTitle(),
            author_name,
            book.GetPublicationYear(),
            tags
        });
    }
    
    // Сортируем книги
    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
        if (a.title != b.title) return a.title < b.title;
        if (a.author_name != b.author_name) return a.author_name < b.author_name;
        return a.publication_year < b.publication_year;
    });
    
    return result;
}

std::vector<detail::BookInfo> View::GetAuthorBooks(const std::string& author_id) const {
    auto books = use_cases_.GetAuthorBooks(author_id);
    std::vector<detail::BookInfo> result;
    result.reserve(books.size());
    
    // Получаем имя автора
    auto authors = use_cases_.GetAuthors();
    std::string author_name = "Unknown";
    for (const auto& author : authors) {
        if (author.GetId().ToString() == author_id) {
            author_name = author.GetName();
            break;
        }
    }
    
    for (const auto& book : books) {
        result.push_back({
            book.GetId().ToString(),
            book.GetTitle(),
            author_name,
            book.GetPublicationYear(),
            {}  // Теги можно добавить позже при необходимости
        });
    }
    
    return result;
}

std::optional<detail::BookInfo> View::EditBookInterface(const std::string& book_id) const {
    auto books = GetCachedBooks();
    auto it = std::find_if(books.begin(), books.end(), 
        [&book_id](const auto& b) { return b.id == book_id; });
    
    if (it == books.end()) {
        return std::nullopt;
    }
    
    auto book = *it;
    bool edit_param = false;
    
    output_ << "Enter new title or empty line to use the current one (" << book.title << "):" << std::endl;
    std::string str;
    if (std::getline(input_, str) && !str.empty()) {
        book.title = str;
        edit_param = true;
    }

    output_ << "Enter publication year or empty line to use the current one (" << book.publication_year << "):" << std::endl;
    if (std::getline(input_, str) && !str.empty()) {
        book.publication_year = std::stoi(str);
        edit_param = true;
    }
    
    output_ << "Enter tags (current tags: ";
    PrintTags(output_, book.tags);
    output_ << "):" << std::endl;
    if (std::getline(input_, str)) {
        book.tags = ParseTags(str);
        edit_param = true;
    }
    
    if (edit_param) {
        return {book};
    }
    return std::nullopt;
}

}  // namespace ui