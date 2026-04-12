#pragma once

#include "http_server.h"
#include "model.h"
#include "game_model.h"
#include "api_handler.h"

#include <boost/asio/strand.hpp>
#include <functional>
#include <optional>
#include <string>
#include <filesystem>

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;

/**
 * @brief Сериализует объект карты в JSON-формат для передачи по HTTP.
 *
 * Преобразует модель карты (модуль model::Map) в строку JSON, содержащую:
 * - id: уникальный идентификатор карты
 * - name: читаемое название
 * - roads: массив дорог (с координатами начала и конца)
 * - buildings: массив зданий (с позицией и размерами)
 * - offices: массив офисов (с координатами и смещениями для UI)
 *
 * ВАЖНО: Этот метод используется для эндпоинта /api/v1/maps/{id}.
 *
 * Пример результата:
 * @code
 * {
 *   "id": "map1",
 *   "name": "First Map",
 *   "roads": [
 *     {"x0": 0, "y0": 0, "x1": 10},
 *     {"x0": 5, "y0": 5, "y1": 20}
 *   ],
 *   "buildings": [
 *     {"x": 5, "y": 5, "w": 30, "h": 20}
 *   ],
 *   "offices": [
 *     {"id": "o1", "x": 100, "y": 200, "offsetX": 5, "offsetY": 0}
 *   ]
 * }
 * @endcode
 *
 * @param map Константная ссылка на карту (не изменяется)
 * @return std::string — сериализованная JSON-строка
 */
std::string MapToJson(const model::Map& map);

/**
 * @brief Сериализует список всех карт в краткий JSON-массив.
 *
 * Используется для эндпоинта GET /api/v1/maps.
 * Возвращает только id и name каждой карты — минимальная информация для отображения списка.
 *
 * Пример:
 * [{"id": "map1", "name": "First Map"}, {"id": "map2", "name": "Second Map"}]
 *
 * @param game Ссылка на игровую модель, содержащую карты
 * @return JSON-строка с массивом объектов {id, name}
 */
std::string MapsListToJson(const model::Game& game);

/**
 * @class RequestHandler
 * @brief Центральный обработчик HTTP-запросов.
 *
 * Основная задача — маршрутизация запросов:
 * - API-запросы (/api/...) → обрабатываются как данные
 * - Статические файлы (/, /css/style.css) → раздаются как файлы
 *
 * Поддерживаемые методы: GET, HEAD.
 * Все остальные — ответ 405 Method Not Allowed.
 *
 * Особенности:
 * - Является "владельцем" ссылки на model::Game — единственный способ доступа к данным.
 * - Не поддерживает копирование (удалены конструктор копирования и оператор присваивания).
 * - Использует шаблонный operator() для совместимости с Boost.Beast.
 *
 * УЧЕБНЫЙ АСПЕКТ:
 * Такой подход позволяет инкапсулировать логику обработки запросов,
 * не раскрывая детали реализации сервера.
 */
class RequestHandler : public std::enable_shared_from_this<RequestHandler> {
public:
    using Strand = net::strand<net::io_context::executor_type>;

    /**
     * @brief Конструктор обработчика.
     * @param game Ссылка на общую модель игры (хранит карты)
     * @param static_files_root Путь к папке со статическими файлами (HTML, CSS, JS)
     * @param api_strand Strand для последовательной обработки API запросов
     *
     * Пример: если static_files_root = "/var/www", то запрос /index.html
     * будет направлен на файл /var/www/index.html.
     */
    RequestHandler(model::Game& game, const std::filesystem::path& static_files_root, Strand api_strand);

    // Инвалидация кэша при изменении карт (если понадобится)
    void InvalidateCache();

    // Запрещаем копирование — объект должен быть единственным
    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    /**
     * @brief Шаблонный вызываемый оператор — точка входа для запроса.
     *
     * Вызывается фреймворком Boost.Beast при получении HTTP-запроса.
     *
     * Параметры шаблона:
     * - Body: тип тела запроса (например, string_body, empty_body)
     * - Allocator: аллокатор заголовков
     * - Send: функция обратного вызова для отправки ответа
     *
     * Логика:
     * 1. Извлекает target (URI) из запроса.
     * 2. Проверяет, относится ли запрос к API
     * 3. API запросы выполняются внутри strand для потокобезопасности
     * 4. Запросы к статическим файлам обрабатываются параллельно
     *
     * @param req Полученный HTTP-запрос
     * @param send Функция: принимает response и отправляет клиенту
     */
    template <typename Body, typename Allocator, typename Send>
    void operator()(
        http::request<Body, http::basic_fields<Allocator>>&& req,
        Send&& send) {
        std::string target(req.target());
        if (!target.empty() && target[0] == '/') {
            target = target.substr(1);  // убираем начальный '/'
        }

        // Проверяем, относится ли запрос к API
        if (IsApiRequest(target)) {
            // Выполняем API запрос внутри strand для потокобезопасности
            auto handle = [self = shared_from_this(), 
                          send = std::forward<Send>(send),
                          req = std::forward<decltype(req)>(req),
                          target = std::move(target)]() mutable {
                self->HandleApiRequest(std::move(req), std::move(send), target);
            };
            return net::dispatch(api_strand_, std::move(handle));
        }

        // Запросы к статическим файлам обрабатываем напрямую
        HandleStaticFileRequest(std::move(req), std::forward<Send>(send), target);
    }

private:
    model::Game& game_;                    ///< Ссылка на модель игры
    std::filesystem::path static_files_root_;  ///< Корень статики (например, ./static)
    Strand api_strand_;                    ///< Strand для обработки API запросов
    game_model::GameSessions sessions_;    ///< Менеджер игровых сеансов
    std::unique_ptr<api_handler::ApiHandler> api_handler_;  ///< Обработчик API запросов

    // === КЭШИРОВАНИЕ ===
    mutable std::unordered_map<std::string, std::string> map_json_cache_;
    mutable std::string maps_list_json_;
    mutable bool maps_list_cache_valid_ = false;

    /// Проверяет, является ли запрос API запросом
    static bool IsApiRequest(std::string_view target);

    /// Обработка API запроса внутри strand
    template <typename Body, typename Allocator, typename Send>
    void HandleApiRequest(
        http::request<Body, http::basic_fields<Allocator>>&& req,
        Send&& send,
        std::string_view target) {
        // Проверяем, что мы внутри strand
        assert(api_strand_.running_in_this_thread());

        // Преобразуем запрос в string_body для api_handler
        http::request<http::string_body> string_req;
        string_req.method(req.method());
        string_req.target(req.target());
        string_req.version(req.version());
        string_req.keep_alive(req.keep_alive());
        
        // Копируем заголовки
        for (const auto& field : req.base()) {
            string_req.set(field.name(), field.value());
        }
        
        // Копируем тело
        string_req.body() = std::string(req.body());
        string_req.prepare_payload();

        // Пробуем обработать через api_handler
        if (auto response = api_handler_->HandleRequest(string_req)) {
            send(std::move(*response));
            return;
        }

        // Старые API эндпоинты /api/v1/maps
        if (target.rfind("api/v1/maps", 0) == 0) {
            send(HandleApiMaps(string_req, target.substr(11)));
            return;
        }

        // Другие API пути — запрещены
        send(MakeBadRequestResponse(string_req));
    }

    /// Обработка запроса к статическому файлу (без strand)
    template <typename Body, typename Allocator, typename Send>
    void HandleStaticFileRequest(
        http::request<Body, http::basic_fields<Allocator>>&& req,
        Send&& send,
        std::string_view target) {
        // Преобразуем запрос в string_body для совместимости
        http::request<http::string_body> string_req;
        string_req.method(req.method());
        string_req.target(req.target());
        string_req.version(req.version());
        string_req.keep_alive(req.keep_alive());
        
        for (const auto& field : req.base()) {
            string_req.set(field.name(), field.value());
        }
        
        string_req.body() = std::string(req.body());
        string_req.prepare_payload();

        send(HandleStaticFile(string_req, target));
    }

    /**
     * @brief Универсальный шаблон для создания HTTP-ответа.
     *
     * Инкапсулирует общую логику:
     * - Установка статуса
     * - Content-Type
     * - Content-Length
     * - keep-alive
     * - Тело (не отправляется при HEAD)
     *
     * @tparam Body Тип тела исходного запроса
     * @param status Код ответа (200, 404 и т.д.)
     * @param body Тело ответа (может быть пустым для HEAD)
     * @param content_type MIME-тип содержимого
     * @param req Исходный запрос (для keep_alive)
     * @return Готовый HTTP-ответ
     */
    template<typename Body>
    http::response<http::string_body> MakeResponse(
        http::status status,
        std::string_view body,
        std::string_view content_type,
        const http::request<Body>& req) {
        http::response<http::string_body> response;
        response.result(status);
        response.set(http::field::content_type, content_type);
        response.content_length(body.size());
        response.keep_alive(req.keep_alive());

        // Для HEAD запросов не добавляем тело
        if (req.method() != http::verb::head) {
            response.body() = std::string(body);
        }

        return response;
    }

    // === API Обработчики ===

    /**
     * @brief Обрабатывает часть пути после /api/v1/maps
     * @param req Исходный запрос
     * @param send Callback для отправки ответа
     * @param path_suffix Часть пути после "api/v1/maps" (например, "/map1")
     */
    http::response<http::string_body> HandleApiMaps(const http::request<http::string_body>& req, std::string_view path_suffix);

    /// Обрабатывает GET /api/v1/maps — возвращает список карт
    http::response<http::string_body> HandleMapsList(const http::request<http::string_body>& req);

    /// Обрабатывает GET /api/v1/maps/{id} — возвращает полное описание карты
    http::response<http::string_body> HandleMapById(const http::request<http::string_body>& req, std::string_view map_id);

    // === Статические файлы ===

    /// Обрабатывает запрос к статическому файлу (HTML, CSS, JS и т.д.)
    http::response<http::string_body> HandleStaticFile(
        const http::request<http::string_body>& req,
        std::string_view target);

    /// Определяет MIME-тип по расширению файла
    std::string GetMimeType(std::string_view path);

    /// Декодирует URL-кодированные символы (%20 → пробел, + → пробел)
    std::string UrlDecode(std::string_view str);

    /// Проверяет, что путь находится внутри корневой директории (защита от path traversal)
    bool IsSubPath(const std::filesystem::path& path) const;

    /// Читает содержимое файла в строку
    std::optional<std::string> ReadFileContent(const std::filesystem::path& path) const;

    /// Создаёт ответ с содержимым файла
    http::response<http::string_body> MakeFileResponse(
        const std::string& content,
        const std::filesystem::path& path,
        const http::request<http::string_body>& req);

    /// Создаёт ответ с ошибкой 400 Bad Request
    http::response<http::string_body> MakeBadRequestResponse(const http::request<http::string_body>& req);

    /// Создаёт ответ с ошибкой 404 Not Found
    http::response<http::string_body> MakeNotFoundResponse(const http::request<http::string_body>& req);

    /// Создаёт ответ с ошибкой 405 Method Not Allowed
    http::response<http::string_body> MakeMethodNotAllowedResponse(const http::request<http::string_body>& req);
};

}  // namespace http_handler