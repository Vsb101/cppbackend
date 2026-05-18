#pragma once
#include <string>
#include <vector>
#include <iostream>

namespace info {

struct AddBookParams {
    std::string title;
    std::string author_id;
    std::string author_name;
    int publication_year = 0;
};

struct AuthorInfo {
    std::string id;
    std::string name;
};

struct TagInfo {
    std::string tag;
};

using Authors = std::vector<AuthorInfo>;
using Tags = std::vector<TagInfo>;

struct BookInfo {
    std::string title;
    int publication_year;
    std::string author_name;
    std::vector<std::string> tags;
    std::string id;  
};

using Books = std::vector<BookInfo>;

inline std::ostream& operator<<(std::ostream& out, const AuthorInfo& author) {
    out << author.name;
    return out;
}

inline std::ostream& operator<<(std::ostream& out, const BookInfo& book) {
    out << book.title << ", " << book.publication_year;
    return out;
}

}  // namespace info
