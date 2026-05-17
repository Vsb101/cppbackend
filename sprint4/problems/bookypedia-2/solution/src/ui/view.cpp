#include "view.h"

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string.hpp>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <sstream>

#include "../app/use_cases.h"
#include "../menu/menu.h"

using namespace std::literals;
namespace ph = std::placeholders;

namespace ui {

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
        menu_.AddAction( "DeleteAuthor"s, "name"s, "Delete author"s
            , [this](auto& cmd_input) { return DeleteAuthor(cmd_input); }
        );
        menu_.AddAction( "EditAuthor"s, "name"s, "Edit author"s
            , [this](auto& cmd_input) { return EditAuthor(cmd_input); }
        );
        menu_.AddAction("AddBook"s, "<pub year> <title>"s, "Adds book"s,
                        std::bind(&View::AddBook, this, ph::_1));
        menu_.AddAction("ShowAuthors"s, {}, "Show authors"s, std::bind(&View::ShowAuthors, this));
        menu_.AddAction("ShowBooks"s, {}, "Show books"s, std::bind(&View::ShowBooks, this));
        menu_.AddAction("ShowBook"s, "title"s, "Show book"s, std::bind(&View::ShowBook, this, ph::_1));
        menu_.AddAction("DeleteBook"s, "title"s, "Delete book"s, std::bind(&View::DeleteBook, this, ph::_1));
        menu_.AddAction("EditBook"s, "title"s, "Edit book"s, std::bind(&View::EditBook, this, ph::_1));
        menu_.AddAction("ShowAuthorBooks"s, {}, "Show author books"s,
                        std::bind(&View::ShowAuthorBooks, this));
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
            auto author = use_cases_.GetAuthorByName(name);
            if (!author) {
                throw std::logic_error("");
            }
            id = author->id;
        }
        return id;
    } 

    std::string View::ParsingTitleBookOrSelect(std::istream& cmd_input) const {
        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);
        std::string book_id;
        if (title.empty()) {
            book_id = SelectBook(GetBooks(), false);
        } else {
            book_id = SelectBook(use_cases_.GetBooksByTitle(title), true);
        }
        return book_id;
    } 

    bool View::DeleteAuthor(std::istream& cmd_input) const {
        try {
            std::string author_id = ParsingNameAuthorOrSelect(cmd_input); 
            if (author_id.empty()) {
                return true;
            }
            use_cases_.DeleteAuthor(author_id);
        } catch (const std::exception&) {
            output_ << "Failed to delete author"sv << std::endl;
        }
        return true;
    }

/*             if (name.empty()) {
                auto author_info = SelectAuthor();
                if (author_info)
                    id = author_info->id;
                else {
                    return true;
                }
            } else {
                auto author = use_cases_.GetAuthorByName(name);
                if (!author) {
                    throw std::logic_error("");
                }
                id = author->id;
            }
 */
    bool View::EditAuthor(std::istream& cmd_input) const {
        try {
            std::string author_id = ParsingNameAuthorOrSelect(cmd_input); 
            if (author_id.empty()) {
                return true;
            }
            output_ << "Enter new name:" << std::endl;
            std::string new_name;
            // Если пользователь ничего не ввел - выходим 
            if (!std::getline(input_, new_name) || new_name.empty()) {
                return true;
            }
            use_cases_.EditAuthor({.id = author_id, .name = new_name});
        } catch (const std::exception&) {
            output_ << "Failed to edit author"sv << std::endl;
        }
        return true;
    }


    bool View::AddBook(std::istream& cmd_input) const {
        try {
            if (auto params = GetBookParams(cmd_input)) {
                auto book_tags = EnterBookTags();
                use_cases_.AddBook(params->author_id, params->author_name, params->title
                    , params->publication_year, book_tags);
            }
        } catch (const std::exception&) {
            output_ << "Failed to add book"sv << std::endl;
        }
        return true;
    }

    bool View::ShowAuthors() const {
        PrintVector(output_, GetAuthors());
        return true;
    }

    void PrintBooks(std::ostream& out, const info::Books& books) {
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

    void PrintBook(std::ostream& out, const info::BookInfo& book) {
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
        PrintBooks(output_, GetBooks());
        return true;
    }

    bool View::ShowBook(std::istream& cmd_input) const {
        try {
            std::string book_id = ParsingTitleBookOrSelect(cmd_input); 
            if (book_id.empty()) {
                return true;
            }
            PrintBook(output_, use_cases_.GetBook(book_id));
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
            const auto book = EditBookInterface(book_id); 
            if (book) {
                use_cases_.EditBook(*book);
            }
        } catch (const std::exception& e) {
            output_ << e.what() << std::endl;
        }
        return true;
    }

    bool View::ShowAuthorBooks() const {
        // TODO: handle error
        try {
            if (auto author_id = SelectAuthor()) {
                PrintVector(output_, GetAuthorBooks(author_id->id));
            }
        } catch (const std::exception&) {
            throw std::runtime_error("Failed to Show Books");
        }
        return true;
    }

    std::optional<info::AddBookParams> View::GetBookParams(std::istream& cmd_input) const {
        info::AddBookParams params;

        cmd_input >> params.publication_year;
        std::getline(cmd_input, params.title);
        boost::algorithm::trim(params.title);

        if (params.publication_year == 0 || params.title.empty()) {
            return std::nullopt;
        }
        auto author_info = EnterAuthor();
        if(author_info.has_value()) {
            params.author_id = author_info->id;
            params.author_name = author_info->name;
            return params;
        }
        
        
        author_info = SelectAuthor();
        if (not author_info.has_value())
            return std::nullopt;
        else {
            params.author_id = author_info->id;
            return params;
        }
    }

    std::optional<info::AuthorInfo> View::SelectAuthor() const {
        output_ << "Select author:" << std::endl;
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
        if (author_idx < 0 or author_idx >= authors.size()) {
            throw std::runtime_error("Invalid author num");
        }

        return {{.id = authors[author_idx].id}};
    }

    std::string View::SelectBook(info::Books books, bool auto_select_one_book) const {
        // auto books = GetBooks();
            // auto books = use_cases_.GetBooksByTitle(title);
        // std::string book_id;

        if (books.empty()) {
            // if (!auto_select_one_book) {
                throw std::logic_error("Book not found");
            // }
            return {};
        } else if (books.size() == 1 && auto_select_one_book) {
            return books.at(0).id;
        } 

        PrintBooks(output_, books);
        output_ << "Enter the book # or empty line to cancel:" << std::endl;

        std::string str;
        if (!std::getline(input_, str) || str.empty()) {
            throw std::logic_error("Book not found");
            // return {};
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

    std::optional<info::AuthorInfo> View::EnterAuthor() const {
        // Приглашение ввести имя автора
        output_ << "Enter author name or empty line to select from list:" << std::endl;
        std::string name;
        // Если пользователь ничего не ввел - выходим и ничено не возвращаем
        if (!std::getline(input_, name) || name.empty()) {
            return std::nullopt;
        }
        // Если П ввел имя автора ищем его в базе
        // Ищем автора в базе по имени
        auto author = GetAuthorByName(name);
        if (author) {
            return author;
        }

        std::string str;
        output_ << "No author found. Do you want to add " + name + " (y/n)?" << std::endl;
        if (!std::getline(input_, str) || str.empty() || (str != "Y" && str != "y")) {
            throw std::logic_error("The user refused to enter or select the author's name.");
        }
        // Если "да", то далее будем создавать автора с указанным именем
        return {{.name = name}};
    }

    std::vector<std::string> ParseBookTags(std::string str) {
        std::vector<std::string> res;
        // Убираем задублированные пробелы
        while (str.find("  ") != std::string::npos){
            boost::replace_all(str, "  ", " ");
        }
        boost::replace_all(str, ", ", ",");
        boost::replace_all(str, " ,", ",");
        // Делим строку на теги
        boost::split(res, str, boost::is_any_of(","));
        // Удаляем пустые теги
        res.erase(remove(res.begin(), res.end(), ""s), res.end());
        // Удаляем дубликаты тегов в массиве
        std::sort(res.begin(), res.end());
        auto last = std::unique(res.begin(), res.end());
        res.erase(last, res.end());
        return res;
    }

    std::vector<std::string> View::EnterBookTags() const {
        output_ << "Enter tags (comma separated):" << std::endl;
        std::string str, tag;
        std::getline(input_, str);
        return ParseBookTags(str);
    }

    std::vector<info::AuthorInfo> View::GetAuthors() const {
        return use_cases_.GetAuthors();
    }

    std::vector<info::BookInfo> View::GetBooks() const {
        return use_cases_.GetBooks();
    }

    std::vector<info::BookInfo> View::GetAuthorBooks(const std::string& author_id) const {
        return use_cases_.GetAuthorBooks(author_id);
    }

    std::optional<info::AuthorInfo> View::GetAuthorByName(const std::string& author_name) const {
        return use_cases_.GetAuthorByName(author_name);
    }

    std::optional<info::BookInfo> View::EditBookInterface(const std::string& book_id) const{
        auto book = use_cases_.GetBook(book_id);
        bool edit_param = false;
        output_ << "Enter new title or empty line to use the current one ("s + book.title + "):"s << std::endl;
        std::string str;
        if (std::getline(input_, str) && !str.empty()) {
            book.title = str;
            edit_param = true;
        }

        output_ << "Enter publication year or empty line to use the current one ("s + std::to_string(book.publication_year) + "):"s << std::endl;
        if (std::getline(input_, str) && !str.empty()) {
            book.publication_year = std::stoi(str);
            edit_param = true;
        }
        std::stringstream ss;
        // PrintVector
        output_ << "Enter tags (current tags: "s;
        PrintTags(output_, book.tags);
        output_ << "):"s << std::endl;
        if (std::getline(input_, str)/*  && !str.empty() */) {
            book.tags = ParseBookTags(str);
            edit_param = true;
        }
        if (edit_param) {
            return {book};
        }
        return std::nullopt;
    }

}  // namespace ui
