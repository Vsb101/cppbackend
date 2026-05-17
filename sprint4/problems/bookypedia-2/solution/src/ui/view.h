#pragma once
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>
#include "struct_info.h"

namespace menu {
class Menu;
}

namespace app {
class UseCases;
}

namespace ui {

class View {
public:
    View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output);

private:
    bool AddAuthor(std::istream& cmd_input) const;
    std::string ParsingNameAuthorOrSelect(std::istream& cmd_input) const;
    std::string ParsingTitleBookOrSelect(std::istream& cmd_input) const;
    bool DeleteAuthor(std::istream& cmd_input) const;
    bool EditAuthor(std::istream& cmd_input) const;
    bool AddBook(std::istream& cmd_input) const;
    bool ShowAuthors() const;
    bool ShowBooks() const;
    bool ShowBook(std::istream& cmd_input) const;
    bool DeleteBook(std::istream& cmd_input) const;
    bool EditBook(std::istream& cmd_input) const;
    bool ShowAuthorBooks() const;

    std::optional<info::AddBookParams> GetBookParams(std::istream& cmd_input) const;
    std::optional<info::AuthorInfo> SelectAuthor() const;
    std::string SelectBook(info::Books books, bool auto_select_one_book) const;
    std::optional<info::AuthorInfo> EnterAuthor() const;
    std::vector<std::string> EnterBookTags() const;
    std::vector<info::AuthorInfo> GetAuthors() const;
    std::vector<info::BookInfo> GetBooks() const;
    std::vector<info::BookInfo> GetAuthorBooks(const std::string& author_id) const;
    std::optional<info::AuthorInfo> GetAuthorByName(const std::string& author_name) const;
    std::optional<info::BookInfo> EditBookInterface(const std::string& book_id) const;
    menu::Menu& menu_;
    app::UseCases& use_cases_;
    std::istream& input_;
    std::ostream& output_;
};

}  // namespace ui