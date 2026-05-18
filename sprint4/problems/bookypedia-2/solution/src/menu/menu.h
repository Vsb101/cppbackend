#pragma once
#include <functional>
#include <iosfwd>
#include <map>
#include <string>
#include <string_view>

namespace menu {

// Меню команд приложения
// Обрабатывает ввод пользователя и диспетчеризирует команды к соответствующим обработчикам
class Menu {
public:
    // Тип обработчика команды
    // Возвращает false для выхода из программы
    using Handler = std::function<bool(std::istream&)>;

    // input_ - поток ввода (std::cin)
    // output_ - поток вывода (std::cout)
    Menu(std::istream& input, std::ostream& output);

    // Добавить команду
    // action_name - имя команды (например, "AddAuthor")
    // args - описание аргументов (например, "<name>")
    // description - описание команды для помощи
    // handler - функция-обработчик
    void AddAction(std::string action_name, std::string args, std::string description,
                   Handler handler);

    // Запуск цикла обработки команд
    // Возвращает только при команде Exit или ошибке ввода
    void Run();

    // Показать список доступных команд
    void ShowInstructions() const;
    
    // Вывести сообщение об ошибке
    void SendErrorMessage(std::string_view mes) const;

private:
    // Информация о команде
    struct ActionInfo {
        Handler handler;        // Обработчик
        std::string args;       // Описание аргументов
        std::string description; // Описание для помощи
    };

    // Парсинг команды из строки
    // Возвращает false для выхода из программы
    [[nodiscard]] bool ParseCommand(std::istream& input);

    std::istream& input_;
    std::ostream& output_;
    std::map<std::string, ActionInfo> actions_;
};

}  // namespace menu
