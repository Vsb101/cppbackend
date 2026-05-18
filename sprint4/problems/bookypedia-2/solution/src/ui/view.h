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

// View - слой представления, обрабатывает ввод/вывод
// Взаимодействует с UseCases для выполнения бизнес-логики
// и с Menu для регистрации команд
class View {
public:
    // Конструктор
    // menu_ - меню для регистрации команд
    // use_cases_ - бизнес-логика
    // input_ - поток ввода (std::cin)
    // output_ - поток вывода (std::cout)
    View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output);

private:
    // === Команды ===
    
    // Команда AddAuthor: добавить автора
    bool AddAuthor(std::istream& cmd_input) const;
    
    // Ввод имени автора или выбор из списка
    std::string ParsingNameAuthorOrSelect(std::istream& cmd_input) const;
    
    // Ввод названия книги или выбор из списка
    std::string ParsingTitleBookOrSelect(std::istream& cmd_input) const;
    
    // Команда DeleteAuthor: удалить автора
    bool DeleteAuthor(std::istream& cmd_input) const;
    
    // Команда EditAuthor: редактировать автора
    bool EditAuthor(std::istream& cmd_input) const;
    
    // Команда AddBook: добавить книгу
    bool AddBook(std::istream& cmd_input) const;
    
    // Команда ShowAuthors: показать всех авторов
    bool ShowAuthors() const;
    
    // Команда ShowBooks: показать все книги
    bool ShowBooks() const;
    
    // Команда ShowBook: показать книгу (с детальной информацией)
    bool ShowBook(std::istream& cmd_input) const;
    
    // Команда DeleteBook: удалить книгу
    bool DeleteBook(std::istream& cmd_input) const;
    
    // Команда EditBook: редактировать книгу
    bool EditBook(std::istream& cmd_input) const;
    
    // Команда ShowAuthorBooks: показать книги автора
    bool ShowAuthorBooks() const;

    // === Вспомогательные методы ===
    
    // Получить параметры новой книги (год, название, автор)
    std::optional<info::AddBookParams> GetBookParams(std::istream& cmd_input) const;
    
    // Выбрать автора из списка
    std::optional<info::AuthorInfo> SelectAuthor() const;
    
    // Выбрать книгу из списка (если несколько с таким названием)
    // auto_select_one_book - если true и книга одна, выбрать автоматически
    std::string SelectBook(const info::Books& books, bool auto_select_one_book) const;
    
    // Ввести имя автора (с предложением добавить нового)
    std::optional<info::AuthorInfo> EnterAuthor() const;
    
    // Ввести теги для книги
    std::vector<std::string> EnterBookTags() const;
    
    // === Запрос данных ===
    
    std::vector<info::AuthorInfo> GetAuthors() const;
    std::vector<info::BookInfo> GetBooks() const;
    std::vector<info::BookInfo> GetAuthorBooks(const std::string& author_id) const;
    std::optional<info::AuthorInfo> GetAuthorByName(const std::string& author_name) const;
    
    // Интерфейс редактирования книги
    std::optional<info::BookInfo> EditBookInterface(const std::string& book_id) const;

    // Зависимости
    menu::Menu& menu_;
    app::UseCases& use_cases_;
    std::istream& input_;
    std::ostream& output_;
};

}  // namespace ui