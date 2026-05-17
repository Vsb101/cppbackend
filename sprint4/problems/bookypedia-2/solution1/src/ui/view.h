#pragma once
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <chrono>

namespace menu {
class Menu;
}

namespace app {
class UseCases;
}

namespace ui {
namespace detail {

struct AuthorInfo {
    std::string id;
    std::string name;
};

struct BookInfo {
    std::string id;
    std::string title;
    std::string author_name;
    int publication_year;
    std::vector<std::string> tags;
};

}  // namespace detail

class View {
public:
    View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output);

    static void SetCacheEnabled(bool enabled) { cache_enabled_ = enabled; }
    static bool IsCacheEnabled() { return cache_enabled_; }

private:
    bool AddAuthor(std::istream& cmd_input) const;
    bool AddBook(std::istream& cmd_input) const;
    bool ShowAuthors() const;
    bool ShowBooks() const;
    bool ShowBook(std::istream& cmd_input) const;
    bool DeleteAuthor(std::istream& cmd_input) const;
    bool DeleteBook(std::istream& cmd_input) const;
    bool EditAuthor(std::istream& cmd_input) const;
    bool EditBook(std::istream& cmd_input) const;
    bool ShowAuthorBooks() const;

    std::string ParsingNameAuthorOrSelect(std::istream& cmd_input) const;
    std::string ParsingTitleBookOrSelect(std::istream& cmd_input) const;
    
    std::optional<detail::AuthorInfo> SelectAuthor() const;
    std::string SelectBook(const std::vector<detail::BookInfo>& books, bool auto_select_one_book) const;
    std::optional<detail::AuthorInfo> EnterAuthor() const;
    std::vector<std::string> EnterBookTags() const;
    std::optional<detail::BookInfo> EditBookInterface(const std::string& book_id) const;
    struct AddBookParams;
    std::optional<AddBookParams> GetBookParams(std::istream& cmd_input) const;
    
    std::vector<detail::AuthorInfo> GetAuthors() const;
    std::vector<detail::BookInfo> GetBooks() const;
    std::vector<detail::BookInfo> GetAuthorBooks(const std::string& author_id) const;
    std::optional<detail::AuthorInfo> GetAuthorByName(const std::string& author_name) const;
    detail::BookInfo GetBookById(const std::string& book_id) const;
    
    void InvalidateCache() const;
    const std::vector<detail::AuthorInfo>& GetCachedAuthors() const;
    const std::vector<detail::BookInfo>& GetCachedBooks() const;

    menu::Menu& menu_;
    app::UseCases& use_cases_;
    std::istream& input_;
    std::ostream& output_;
    
    mutable std::optional<std::vector<detail::AuthorInfo>> cached_authors_;
    mutable std::optional<std::vector<detail::BookInfo>> cached_books_;
    mutable std::chrono::steady_clock::time_point last_cache_update_;
    
    inline static bool cache_enabled_ = false;
};

}  // namespace ui