#pragma once

#include "http_server.h"
#include "model.h"

#include <functional>
#include <optional>
#include <string>
#include <filesystem>
#include <array>

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;

// === ТИПЫ ОШИБОК API ===

/**
 * @brief Перечисление кодов ошибок API.
 *
 * Каждому значению соответствует HTTP-статус, строковый код и сообщение.
 */
enum class ApiError {
    BAD_REQUEST,        ///< Некорректный запрос (400)
    METHOD_NOT_ALLOWED, ///< Метод не поддерживается (405)
    MAP_NOT_FOUND,      ///< Карта не найдена (404)
    FILE_NOT_FOUND      ///< Файл не найден (404)
};

namespace detail {

struct ApiErrorInfo {
    http::status status;       ///< HTTP-статус ответа
    std::string_view code;     ///< Строковый код ошибки (для клиента)
    std::string_view message;  ///< Человекочитаемое описание
};

// Таблица соответствия enum → статус, код, сообщение
constexpr std::array<ApiErrorInfo, 4> api_errors = {{
    {http::status::bad_request,        "badRequest",        "Bad request"},
    {http::status::method_not_allowed, "methodNotAllowed",  "Method not allowed"},
    {http::status::not_found,          "mapNotFound",       "Map not found"},
    {http::status::not_found,          "fileNotFound",      "File not found"},
}};

/**
 * @brief Возвращает информацию об ошибке по её типу.
 */
inline const ApiErrorInfo& GetErrorInfo(ApiError error) {
    return api_errors[static_cast<size_t>(error)];
}

// Проверка согласованности enum и таблицы
static_assert(static_cast<size_t>(ApiError::BAD_REQUEST) == 0);
static_assert(static_cast<size_t>(ApiError::METHOD_NOT_ALLOWED) == 1);
static_assert(static_cast<size_t>(ApiError::MAP_NOT_FOUND) == 2);
static_assert(static_cast<size_t>(ApiError::FILE_NOT_FOUND) == 3);

} // namespace detail

/**
 * @class RequestHandler
 * @brief Обработчик HTTP-запросов.
 *
 * Центральный компонент, отвечающий за маршрутизацию и обработку запросов:
 * - API-эндпоинты (/api/...) → возвращают JSON-данные
 * - Статические файлы (/, /css/style.css) → раздаются как файлы
 *
 * Поддерживаемые методы: GET, HEAD. Остальные — 405 Method Not Allowed.
 *
 * Особенности:
 * - Владеет ссылкой на model::Game — единственный источник данных.
 * - Не поддерживает копирование.
 * - Использует шаблонный operator() для интеграции с Boost.Beast.
 */
class RequestHandler {
public:
    /**
     * @brief Конструктор обработчика.
     * @param game Ссылка на игровую модель
     * @param static_files_root Путь к директории со статическими файлами
     *
     * Пример: если static_files_root = "./static", то запрос "/index.html"
     * будет направлен на "./static/index.html".
     */
    explicit RequestHandler(model::Game& game, const std::filesystem::path& static_files_root);

    /**
     * @brief Инвалидирует кэш сериализованных данных.
     *
     * Вызывается при изменении карт, чтобы обновить закэшированные JSON.
     */
    void InvalidateCache();

    // Запрещаем копирование
    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    /**
     * @brief Шаблонный вызываемый оператор — точка входа для HTTP-запроса.
     *
     * Вызывается фреймворком Boost.Beast при получении запроса.
     *
     * @tparam Body Тип тела запроса
     * @tparam Allocator Аллокатор заголовков
     * @tparam Send Функция обратного вызова для отправки ответа
     * @param req Полученный HTTP-запрос
     * @param send Функция отправки ответа клиенту
     */
    template <typename Body, typename Allocator, typename Send>
    void operator()(
        http::request<Body, http::basic_fields<Allocator>>&& req,
        Send&& send) {
        std::string target(req.target());
        if (!target.empty() && target[0] == '/') {
            target = target.substr(1);  // убираем начальный '/'
        }

        // Поддерживаем только GET и HEAD
        if (req.method() != http::verb::get && req.method() != http::verb::head) {
            send(MakeMethodNotAllowedResponse(req));
            return;
        }

        // API: /api/v1/maps
        if (target.rfind("api/v1/maps", 0) == 0) {
            HandleApiMaps(std::move(req), std::forward<Send>(send), target.substr(11));
        }
        // Другие API-пути — запрещены
        else if (target.rfind("api/", 0) == 0) {
            send(MakeApiErrorResponse(req, ApiError::BAD_REQUEST));
        }
        // Статические файлы
        else {
            HandleStaticFile(req, std::forward<Send>(send), target);
        }
    }

private:
    // === ДАННЫЕ ===
    model::Game& game_;                    ///< Ссылка на модель игры
    std::filesystem::path static_files_root_;  ///< Корень статических файлов

    // === КЭШИРОВАНИЕ ===
    mutable std::unordered_map<std::string, std::string> map_json_cache_;  ///< Кэш JSON по ID карты
    mutable std::string maps_list_json_;       ///< Кэш списка карт
    mutable bool maps_list_cache_valid_ = false;  ///< Флаг валидности кэша

    // === HTTP: УНИВЕРСАЛЬНЫЕ ОТВЕТЫ ===
    /**
     * @brief Создаёт HTTP-ответ с указанным статусом и телом.
     *
     * Устанавливает:
     * - Content-Type
     * - Content-Length
     * - keep-alive
     * - Тело (не отправляется при HEAD)
     *
     * @tparam Body Тип тела исходного запроса
     * @param status HTTP-статус
     * @param body Тело ответа
     * @param content_type MIME-тип содержимого
     * @param req Исходный запрос
     * @return Готовый HTTP-ответ
     */
    template<typename Body>
    http::response<http::string_body> MakeResponse(
        http::status status,
        std::string_view body,
        std::string_view content_type,
        const http::request<Body>& req);

    /**
     * @brief Создаёт JSON-ответ с ошибкой API.
     *
     * Формат тела: {"code": "...", "message": "..."}
     *
     * @tparam Body Тип тела запроса
     * @param req Исходный запрос
     * @param error Тип ошибки
     * @return HTTP-ответ с JSON-ошибкой
     */
    template<typename Body>
    http::response<http::string_body> MakeApiErrorResponse(
        const http::request<Body>& req, ApiError error);

    /**
     * @brief Создаёт ответ 405 Method Not Allowed.
     *
     * Добавляет заголовок Allow: GET, HEAD.
     *
     * @tparam Body Тип тела запроса
     * @param req Исходный запрос
     * @return HTTP-ответ с ошибкой
     */
    template<typename Body>
    http::response<http::string_body> MakeMethodNotAllowedResponse(
        const http::request<Body>& req);

    // === API ОБРАБОТЧИКИ ===
    /**
     * @brief Обрабатывает запросы к /api/v1/maps.
     *
     * Роутинг:
     * - /api/v1/maps - HandleMapsList
     * - /api/v1/maps/{id} - HandleMapById
     */
    void HandleApiMaps(
        http::request<http::string_body>&& req,
        std::function<void(http::response<http::string_body>&&)>&& send,
        std::string_view path_suffix);

    /**
     * @brief Обрабатывает GET /api/v1/maps — возвращает список карт.
     * @return JSON-ответ с массивом {id, name}
     */
    http::response<http::string_body> HandleMapsList(const http::request<http::string_body>& req);

    /**
     * @brief Обрабатывает GET /api/v1/maps/{id} — возвращает полное описание карты.
     * @param req Исходный HTTP-запрос
     * @param map_id Идентификатор карты
     * @return JSON-ответ с данными карты или ошибка 404
     */
    http::response<http::string_body> HandleMapById(const http::request<http::string_body>& req, std::string_view map_id);

    // === СТАТИЧЕСКИЕ ФАЙЛЫ ===
    /**
     * @brief Обрабатывает запрос к статическим файлам.
     *
     * Поддерживает index.html по умолчанию, защиту от path traversal.
     */
    void HandleStaticFile(
        const http::request<http::string_body>& req,
        std::function<void(http::response<http::string_body>&&)>&& send,
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
};

}  // namespace http_handler