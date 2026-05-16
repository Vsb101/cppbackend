#include "view.h"

#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <charconv>
#include <format>
#include <iostream>
#include <map>
#include <sstream>

#include "../app/use_cases.h"
#include "../menu/menu.h"

using namespace std::literals;
namespace ph = std::placeholders;

namespace ui {
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
    for (const auto& value : vector) {
        out << i++ << " " << value << std::endl;
    }
}

static bool IsNumber(std::string_view str) {
    if (str.empty()) return false;
    return std::ranges::all_of(str, ::isdigit);
}

static std::vector<std::string> ParseTags(std::string_view input) {
    std::vector<std::string> tags;
    if (input.empty()) return tags;
    
    std::istringstream ss{std::string(input)};
    for (std::string tag; std::getline(ss, tag, ','); ) {
        boost::algorithm::trim(tag);
        if (!tag.empty()) tags.push_back(std::move(tag));
    }
    return tags;
}

static std::optional<int> ParseIndex(std::string_view str) {
    if (str.empty()) return std::nullopt;
    
    int result;
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), result);
    if (ec == std::errc{} && ptr == str.data() + str.size()) {
        return result - 1;
    }
    return std::nullopt;
}

static std::vector<detail::BookInfo> FindBooksByTitle(
    const std::vector<detail::BookInfo>& books, 
    std::string_view title)
{
    std::vector<detail::BookInfo> result;
    result.reserve(books.size() / 10);
    for (const auto& book : books) {
        if (book.title == title) result.push_back(book);
    }
    return result;
}

View::View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output)
    : menu_{menu}, use_cases_{use_cases}, input_{input}, output_{output} {
    menu_.AddAction("AddAuthor"s, "name"s, "Adds author"s, std::bind(&View::AddAuthor, this, ph::_1));
    menu_.AddAction("AddBook"s, "<pub year> <title>"s, "Adds book"s, std::bind(&View::AddBook, this, ph::_1));
    menu_.AddAction("ShowAuthors"s, {}, "Show authors"s, std::bind(&View::ShowAuthors, this));
    menu_.AddAction("ShowBooks"s, {}, "Show books"s, std::bind(&View::ShowBooks, this));
    menu_.AddAction("ShowBook"s, "<title> or <book#>"s, "Show book details"s, std::bind(&View::ShowBook, this, ph::_1));
    menu_.AddAction("DeleteAuthor"s, "<name> or <author#>"s, "Delete author"s, std::bind(&View::DeleteAuthor, this, ph::_1));
    menu_.AddAction("DeleteBook"s, "<title> or <book#>"s, "Delete book"s, std::bind(&View::DeleteBook, this, ph::_1));
    menu_.AddAction("EditAuthor"s, "<name> or <author#>"s, "Edit author"s, std::bind(&View::EditAuthor, this, ph::_1));
    menu_.AddAction("EditBook"s, "<title> or <book#>"s, "Edit book"s, std::bind(&View::EditBook, this, ph::_1));
}

bool View::AddAuthor(std::istream& cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        boost::algorithm::trim(name);
        use_cases_.AddAuthor(std::move(name));
    } catch (...) {
        output_ << "Failed to add author"sv << std::endl;
    }
    return true;
}

bool View::AddBook(std::istream& cmd_input) const {
    try {
        int publication_year;
        cmd_input >> publication_year;
        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);

        output_ << "Enter author name or empty line to select from list: " << std::flush;
        std::string author_input;
        std::getline(input_, author_input);
        boost::algorithm::trim(author_input);

        std::string author_id;

        if (author_input.empty()) {
            auto authors = GetAuthors();
            if (authors.empty()) {
                output_ << "No authors found. Failed to add book."sv << std::endl;
                return true;
            }
            PrintVector(output_, authors);
            output_ << "Enter author #" << std::flush;
            std::getline(input_, author_input);
            author_id = std::move(author_input);
        } else {
            auto authors = GetAuthors();
            auto it = std::ranges::find_if(authors, [&author_input](const auto& a) { return a.name == author_input; });
            
            if (it == authors.end()) {
                output_ << std::format("No author found. Do you want to add {} (y/n)? ", author_input) << std::flush;
                std::string response;
                std::getline(input_, response);
                if (response != "y" && response != "Y") {
                    output_ << "Failed to add book."sv << std::endl;
                    return true;
                }
                use_cases_.AddAuthor(author_input);
                author_id = std::move(author_input);
            } else {
                author_id = it->id;
            }
        }

        output_ << "Enter tags (comma separated): " << std::flush;
        std::string tags_input;
        std::getline(input_, tags_input);
        
        use_cases_.AddBookWithAuthorSelection(title, publication_year, author_id, ParseTags(tags_input));

    } catch (...) {
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
    std::string input;
    std::getline(cmd_input, input);
    boost::algorithm::trim(input);
    
    const detail::BookInfo* selected_book = nullptr;
    
    if (input.empty()) {
        if (books.empty()) return true;
        PrintVector(output_, books);
        output_ << "Enter the book # or empty line to cancel: " << std::flush;
        std::getline(input_, input);
        auto idx = ParseIndex(input);
        if (!idx || *idx < 0 || *idx >= static_cast<int>(books.size())) return true;
        selected_book = &books[*idx];
    } else if (IsNumber(input)) {
        auto idx = ParseIndex(input);
        if (!idx || *idx < 0 || *idx >= static_cast<int>(books.size())) return true;
        selected_book = &books[*idx];
    } else {
        auto matching_vec = FindBooksByTitle(books, input);
        if (matching_vec.empty()) return true;
        
        if (matching_vec.size() > 1) {
            PrintVector(output_, matching_vec);
            output_ << "Enter the book # or empty line to cancel: " << std::flush;
            std::getline(input_, input);
            auto idx = ParseIndex(input);
            if (!idx || *idx < 0 || *idx >= static_cast<int>(matching_vec.size())) return true;
            selected_book = &matching_vec[*idx];
        } else {
            selected_book = &matching_vec[0];
        }
    }
    
    if (selected_book) {
        output_ << std::format(
            "Title: {}\nAuthor: {}\nPublication year: {}",
            selected_book->title,
            selected_book->author_name,
            selected_book->publication_year
        );
        if (!selected_book->tags.empty()) {
            output_ << "\nTags: " << selected_book->tags[0];
            for (size_t i = 1; i < selected_book->tags.size(); ++i) {
                output_ << ", " << selected_book->tags[i];
            }
        }
        output_ << std::endl;
    }
    return true;
}

bool View::DeleteAuthor(std::istream& cmd_input) const {
    std::string input;
    std::getline(cmd_input, input);
    boost::algorithm::trim(input);
    
    if (input.empty()) {
        auto authors = GetAuthors();
        if (authors.empty()) return true;
        PrintVector(output_, authors);
        output_ << "Enter author # or empty line to cancel" << std::endl;
        std::getline(input_, input);
        if (input.empty()) return true;
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
    
    std::vector<detail::BookInfo> matching;
    
    if (input.empty()) {
        matching = books;
    } else if (IsNumber(input)) {
        auto idx = ParseIndex(input);
        if (idx && *idx >= 0 && *idx < static_cast<int>(books.size())) {
            matching.push_back(std::move(books[*idx]));
        }
    } else {
        matching = FindBooksByTitle(books, input);
    }
    
    if (matching.empty()) {
        output_ << "Failed to delete book"sv << std::endl;
        return true;
    }
    
    int book_idx;
    if (matching.size() == 1 && !input.empty()) {
        book_idx = 0;
    } else {
        PrintVector(output_, matching);
        output_ << "Enter the book # or empty line to cancel: " << std::flush;
        std::getline(input_, input);
        auto idx = ParseIndex(input);
        if (!idx || *idx < 0 || *idx >= static_cast<int>(matching.size())) return true;
        book_idx = *idx;
    }
    
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
    
    std::string author_input = input;
    
    if (author_input.empty()) {
        output_ << "Select author:" << std::endl;
        if (authors.empty()) return true;
        PrintVector(output_, authors);
        output_ << "Enter author # or empty line to cancel" << std::endl;
        std::getline(input_, author_input);
        if (author_input.empty()) return true;
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
    
    const detail::BookInfo* selected_book = nullptr;
    
    if (input.empty()) {
        if (books.empty()) return true;
        PrintVector(output_, books);
        output_ << "Enter the book # or empty line to cancel: " << std::flush;
        std::getline(input_, input);
        auto idx = ParseIndex(input);
        if (!idx || *idx < 0 || *idx >= static_cast<int>(books.size())) return true;
        selected_book = &books[*idx];
    } else if (IsNumber(input)) {
        auto idx = ParseIndex(input);
        if (!idx || *idx < 0 || *idx >= static_cast<int>(books.size())) {
            output_ << "Book not found"sv << std::endl;
            return true;
        }
        selected_book = &books[*idx];
    } else {
        auto matching_vec = FindBooksByTitle(books, input);
        if (matching_vec.empty()) {
            output_ << "Book not found"sv << std::endl;
            return true;
        }
        
        if (matching_vec.size() > 1) {
            PrintVector(output_, matching_vec);
            output_ << "Enter the book # or empty line to cancel: " << std::flush;
            std::getline(input_, input);
            auto idx = ParseIndex(input);
            if (!idx || *idx < 0 || *idx >= static_cast<int>(matching_vec.size())) return true;
            selected_book = &matching_vec[*idx];
        } else {
            selected_book = &matching_vec[0];
        }
    }
    
    if (!selected_book) {
        output_ << "Book not found"sv << std::endl;
        return true;
    }
    
    output_ << std::format(
        "Enter new title or empty line to use the current one ({}): ",
        selected_book->title
    ) << std::flush;
    std::string new_title;
    std::getline(input_, new_title);
    boost::algorithm::trim(new_title);
    
    output_ << std::format(
        "Enter publication year or empty line to use the current one ({}): ",
        selected_book->publication_year
    ) << std::flush;
    std::string year_input;
    std::getline(input_, year_input);
    boost::algorithm::trim(year_input);
    int new_year = year_input.empty() ? 0 : std::stoi(year_input);
    
    std::string current_tags_str;
    if (!selected_book->tags.empty()) {
        current_tags_str.reserve(selected_book->tags.size() * 10);
        current_tags_str = selected_book->tags[0];
        for (size_t i = 1; i < selected_book->tags.size(); ++i) {
            current_tags_str += ", ";
            current_tags_str += selected_book->tags[i];
        }
    }
    output_ << std::format("Enter tags (current tags: {}): ", current_tags_str) << std::flush;
    std::string tags_input;
    std::getline(input_, tags_input);
    
    auto books_after = GetBooks();
    int book_num = 0;
    for (size_t i = 0; i < books_after.size(); ++i) {
        if (books_after[i].title == selected_book->title && 
            books_after[i].publication_year == selected_book->publication_year) {
            book_num = static_cast<int>(i) + 1;
            break;
        }
    }
    
    if (!use_cases_.EditBook(std::to_string(book_num), new_title, new_year, ParseTags(tags_input))) {
        output_ << "Book not found"sv << std::endl;
    }
    return true;
}

std::vector<detail::AuthorInfo> View::GetAuthors() const {
    try {
        auto authors = use_cases_.GetAuthors();
        std::vector<detail::AuthorInfo> result;
        result.reserve(authors.size());
        for (const auto& author : authors) {
            result.push_back({author.GetId().ToString(), author.GetName()});
        }
        return result;
    } catch (...) {
        throw std::runtime_error("Failed to get authors");
    }
}
        
std::vector<detail::BookInfo> View::GetBooks() const {
    try {
        auto all_books = use_cases_.GetBooks();
        auto authors = use_cases_.GetAuthors();
        
        std::map<std::string, std::string> author_map;
        for (const auto& author : authors) {
            author_map[author.GetId().ToString()] = author.GetName();
        }
        
        std::vector<detail::BookInfo> books;
        books.reserve(all_books.size());
        
        for (const auto& book : all_books) {
            std::string author_name;
            auto it = author_map.find(book.GetAuthorId().ToString());
            if (it != author_map.end()) {
                author_name = it->second;
            }
            
            books.push_back({
                book.GetTitle(),
                std::move(author_name),
                book.GetPublicationYear(),
                book.GetTags()
            });
        }
        
        std::sort(books.begin(), books.end(), [](const auto& a, const auto& b) {
            if (a.title != b.title) return a.title < b.title;
            if (a.author_name != b.author_name) return a.author_name < b.author_name;
            return a.publication_year < b.publication_year;
        });
        
        return books;
    } catch (...) {
        throw std::runtime_error("Failed to get books");
    }
}

}  // namespace ui
