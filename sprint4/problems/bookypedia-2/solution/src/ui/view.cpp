#include "view.h"

#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <charconv>
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
        if (!tag.empty()) {
            while (tag.find("  ") != std::string::npos) {
                boost::algorithm::replace_all(tag, "  ", " ");
            }
            tags.push_back(std::move(tag));
        }
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
    for (const auto& book : books) {
        if (book.title == title) result.push_back(book);
    }
    return result;
}

View::View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output)
    : menu_{menu}, use_cases_{use_cases}, input_{input}, output_{output} {
    menu_.AddAction("AddAuthor"s, "name"s, "Adds author"s, [this](std::istream& is) { return AddAuthor(is); });
    menu_.AddAction("AddBook"s, "<pub year> <title>"s, "Adds book"s, [this](std::istream& is) { return AddBook(is); });
    menu_.AddAction("ShowAuthors"s, {}, "Show authors"s, [this](std::istream&) { return ShowAuthors(); });
    menu_.AddAction("ShowBooks"s, {}, "Show books"s, [this](std::istream&) { return ShowBooks(); });
    menu_.AddAction("ShowBook"s, "<title> or <book#>"s, "Show book details"s, [this](std::istream& is) { return ShowBook(is); });
    menu_.AddAction("DeleteAuthor"s, "<name> or <author#>"s, "Delete author"s, [this](std::istream& is) { return DeleteAuthor(is); });
    menu_.AddAction("DeleteBook"s, "<title> or <book#>"s, "Delete book"s, [this](std::istream& is) { return DeleteBook(is); });
    menu_.AddAction("EditAuthor"s, "<name> or <author#>"s, "Edit author"s, [this](std::istream& is) { return EditAuthor(is); });
    menu_.AddAction("EditBook"s, "<title> or <book#>"s, "Edit book"s, [this](std::istream& is) { return EditBook(is); });
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
        if (!(cmd_input >> publication_year)) {
            output_ << "Failed to add book"sv << std::endl;
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

        if (author_input.empty()) {
            auto authors = GetAuthors();
            if (authors.empty()) {
                output_ << "No authors found. Failed to add book."sv << std::endl;
                return true;
            }
            output_ << "Select author:"sv << std::endl;
            PrintVector(output_, authors);
            output_ << std::endl;
            output_ << "Enter author #" << std::endl;
            std::getline(input_, author_input);
            final_author_input = std::move(author_input);
        } else {
            auto authors = GetAuthors();
            auto it = std::find_if(authors.begin(), authors.end(), [&author_input](const auto& a) { 
                return a.name == author_input; 
            });
            
            if (it == authors.end()) {
                output_ << "No author found. Do you want to add " << author_input << " (y/n)? " << std::flush;
                
                std::string response;
                std::getline(input_, response);
                boost::algorithm::trim(response);
                if (response != "y" && response != "Y") {
                    output_ << "Failed to add book."sv << std::endl;
                    return true;
                }
            }
        }

        output_ << "Enter tags (comma separated):" << std::endl;
        
        std::string tags_input;
        std::getline(input_, tags_input);
        
        use_cases_.AddBookWithAuthorSelection(title, publication_year, final_author_input, ParseTags(tags_input));

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
    auto all_books = GetBooks();
    std::string input;
    std::getline(cmd_input, input);
    boost::algorithm::trim(input);

    std::vector<detail::BookInfo> matching_books;

    if (input.empty()) {
        if (all_books.empty()) return true;
        output_ << "Select book:"sv << std::endl;
        output_ << std::endl;
        PrintVector(output_, all_books);
        output_ << std::endl;
        output_ << "Enter the book # or empty line to cancel: " << std::flush;
        std::getline(input_, input);
        boost::algorithm::trim(input);
        if (input.empty()) return true;
        
        auto idx = ParseIndex(input);
        if (!idx || *idx < 0 || *idx >= static_cast<int>(all_books.size())) return true;
        matching_books.push_back(all_books[*idx]);
    } else if (IsNumber(input)) {
        auto idx = ParseIndex(input);
        if (!idx || *idx < 0 || *idx >= static_cast<int>(all_books.size())) {
            output_ << "Book not found"sv << std::endl;
            return true;
        }
        matching_books.push_back(all_books[*idx]);
    } else {
        matching_books = FindBooksByTitle(all_books, input);
        if (matching_books.empty()) {
            output_ << "Book not found"sv << std::endl;
            return true;
        }

        if (matching_books.size() > 1) {
            output_ << std::endl;
            PrintVector(output_, matching_books);
            output_ << std::endl;
            output_ << "Enter the book # or empty line to cancel: " << std::flush;
            std::getline(input_, input);
            boost::algorithm::trim(input);
            if (input.empty()) return true;

            auto idx = ParseIndex(input);
            if (!idx || *idx < 0 || *idx >= static_cast<int>(matching_books.size())) return true;
            matching_books = {matching_books[*idx]};
        }
    }

    if (!matching_books.empty()) {
        const auto& book = matching_books[0];
        output_ << "Title: " << book.title << "\n";
        output_ << "Author: " << book.author_name << "\n";
        output_ << "Publication year: " << book.publication_year;
        
        if (!book.tags.empty()) {
            output_ << "\nTags: " << book.tags[0];
            for (size_t i = 1; i < book.tags.size(); ++i) {
                output_ << ", " << book.tags[i];
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
        output_ << std::endl;
        PrintVector(output_, authors);
        output_ << std::endl;
        output_ << "Enter author # or empty line to cancel" << std::endl;
        std::getline(input_, input);
        boost::algorithm::trim(input);
        if (input.empty()) return true;
    }

    if (!use_cases_.DeleteAuthor(input)) {
        output_ << "Failed to delete author"sv << std::endl;
    }
    return true;
}

bool View::DeleteBook(std::istream& cmd_input) const {
    auto all_books = GetBooks();
    std::string input;
    std::getline(cmd_input, input);
    boost::algorithm::trim(input);

    std::vector<detail::BookInfo> matching_books;

    if (input.empty()) {
        matching_books = all_books;
    } else {
        matching_books = FindBooksByTitle(all_books, input);
    }

    if (matching_books.empty()) {
        output_ << "Book not found"sv << std::endl;
        return true;
    }

    std::string target_book_id;

    if (matching_books.size() == 1 && !input.empty()) {
        target_book_id = matching_books[0].id;
    } else {
        output_ << std::endl;
        PrintVector(output_, matching_books);
        output_ << std::endl;
        output_ << "Enter the book # or empty line to cancel: " << std::flush;
        
        std::string choice;
        std::getline(input_, choice);
        boost::algorithm::trim(choice);
        if (choice.empty()) return true;

        auto idx = ParseIndex(choice);
        if (!idx || *idx < 0 || *idx >= static_cast<int>(matching_books.size())) return true;
        target_book_id = matching_books[*idx].id;
    }

    if (!use_cases_.DeleteBook(target_book_id)) {
        output_ << "Failed to delete book"sv << std::endl;
    }
    return true;
}

bool View::EditAuthor(std::istream& cmd_input) const {
    std::string input;
    std::getline(cmd_input, input);
    boost::algorithm::trim(input);

    std::string author_target = input;

    if (author_target.empty()) {
        output_ << "Select author:"sv << std::endl;
        auto authors = GetAuthors();
        if (authors.empty()) return true;
        output_ << std::endl;
        PrintVector(output_, authors);
        output_ << std::endl;
        output_ << "Enter author # or empty line to cancel" << std::endl;
        
        std::getline(input_, author_target);
        boost::algorithm::trim(author_target);
        if (author_target.empty()) return true;
    }

    output_ << "Enter new name:\n" << std::flush;
    std::string new_name;
    std::getline(input_, new_name);
    boost::algorithm::trim(new_name);

    if (!use_cases_.EditAuthor(author_target, new_name)) {
        output_ << "Failed to edit author"sv << std::endl;
    }
    return true;
}

bool View::EditBook(std::istream& cmd_input) const {
    auto all_books = GetBooks();
    std::string input;
    std::getline(cmd_input, input);
    boost::algorithm::trim(input);

    std::vector<detail::BookInfo> matching_books;

    if (input.empty()) {
        matching_books = all_books;
    } else {
        matching_books = FindBooksByTitle(all_books, input);
    }

    if (matching_books.empty()) {
        output_ << "Book not found"sv << std::endl;
        return true;
    }

    std::string target_book_id;

    if (matching_books.size() == 1 && !input.empty()) {
        target_book_id = matching_books[0].id;
    } else {
        output_ << std::endl;
        PrintVector(output_, matching_books);
        output_ << std::endl;
        output_ << "Enter the book # or empty line to cancel: " << std::flush;
        
        std::string choice;
        std::getline(input_, choice);
        boost::algorithm::trim(choice);
        if (choice.empty()) return true;

        auto idx = ParseIndex(choice);
        if (!idx || *idx < 0 || *idx >= static_cast<int>(matching_books.size())) return true;
        target_book_id = matching_books[*idx].id;
    }

    auto book_it = std::find_if(all_books.begin(), all_books.end(), [&target_book_id](const auto& b) {
        return b.id == target_book_id;
    });

    if (book_it == all_books.end()) {
        output_ << "Book not found"sv << std::endl;
        return true;
    }

    const auto& selected_book = *book_it;

    output_ << "Enter new title or empty line to use the current one (" << selected_book.title << "):" << std::endl;
    std::string new_title;
    std::getline(input_, new_title);
    boost::algorithm::trim(new_title);

    output_ << "Enter publication year or empty line to use the current one (" << selected_book.publication_year << "):" << std::endl;
    std::string year_input;
    std::getline(input_, year_input);
    boost::algorithm::trim(year_input);
    int new_year = year_input.empty() ? 0 : std::stoi(year_input);

    std::string current_tags_str;
    if (!selected_book.tags.empty()) {
        current_tags_str = selected_book.tags[0];
        for (size_t i = 1; i < selected_book.tags.size(); ++i) {
            current_tags_str += ", ";
            current_tags_str += selected_book.tags[i];
        }
    }

    output_ << "Enter tags (current tags: " << current_tags_str << "):" << std::endl;
    std::string tags_input;
    std::getline(input_, tags_input);

    if (!use_cases_.EditBook(target_book_id, new_title, new_year, ParseTags(tags_input))) {
        output_ << "Book not found"sv << std::endl;
    }
    return true;
}

std::vector<detail::AuthorInfo> View::GetAuthors() const {
    try {
        auto authors = use_cases_.GetAuthors(); // Порядок строго из БД!
        std::vector<detail::AuthorInfo> result;
        result.reserve(authors.size());
        for (const auto& author : authors) {
            result.push_back({author.GetId().ToString(), author.GetName()});
        }
        // 
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
                book.GetId().ToString(),
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
