#pragma once
#include <string>
#include <vector>
#include <iostream>

namespace info {

// Параметры для добавления книги
struct AddBookParams {
    std::string title;        // Название книги
    std::string author_id;    // ID автора (если выбран из списка)
    std::string author_name;  // Имя автора (если введено вручную)
    int publication_year = 0; // Год публикации
};

// Информация об авторе (для передачи между слоями)
struct AuthorInfo {
    std::string id;   // Уникальный идентификатор
    std::string name; // Имя автора
};

// Информация о теге
struct TagInfo {
    std::string tag; // Текст тега
};

// Алиасы для коллекций
using Authors = std::vector<AuthorInfo>; // Список авторов
using Tags = std::vector<TagInfo>;       // Список тегов

// Информация о книге (для передачи между слоями)
struct BookInfo {
    std::string title;             // Название книги
    int publication_year;          // Год публикации
    std::string author_name;       // Имя автора
    std::vector<std::string> tags; // Список тегов
    std::string id;                // Уникальный идентификатор
    
    // Дефолтный конструктор
    BookInfo() = default;
    
    // Конструктор для быстрого создания без тегов и ID
    BookInfo(std::string t, int year)
        : title(std::move(t))
        , publication_year(year)
        , author_name("")
        , tags()
        , id("") {}
    
    // Конструктор с автором
    BookInfo(std::string t, int year, std::string author)
        : title(std::move(t))
        , publication_year(year)
        , author_name(std::move(author))
        , tags()
        , id("") {}
    
    // Полный конструктор
    BookInfo(std::string t, int year, std::string author, 
             std::vector<std::string> tg, std::string book_id)
        : title(std::move(t))
        , publication_year(year)
        , author_name(std::move(author))
        , tags(std::move(tg))
        , id(std::move(book_id)) {}
};

// Список книг
using Books = std::vector<BookInfo>;

// Оператор вывода для AuthorInfo
inline std::ostream& operator<<(std::ostream& out, const AuthorInfo& author) {
    out << author.name;
    return out;
}

// Оператор вывода для BookInfo
inline std::ostream& operator<<(std::ostream& out, const BookInfo& book) {
    out << book.title << ", " << book.publication_year;
    return out;
}

}  // namespace info
