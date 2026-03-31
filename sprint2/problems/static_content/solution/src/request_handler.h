#pragma once

#include "http_server.h"
#include "model.h"

#include <functional>
#include <optional>
#include <string>
#include <filesystem>

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;

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
class RequestHandler {
public:
    /**
     * @brief Конструктор обработчика.
     * @param game Ссылка на общую модель игры (хранит карты)
     * @param static_files_root Путь к папке со статическими файлами (HTML, CSS, JS)
     *
     * Пример: если static_files_root = "/var/www", то запрос /index.html
     * будет направлен на файл /var/www/index.html.
     */
    explicit RequestHandler(model::Game& game, const std::filesystem::path& static_files_root);

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
     * 2. Проверяет метод: только GET и HEAD разрешены.
     * 3. Роутинг:
     *    - /api/v1/maps → HandleMapsList или HandleMapById
     *    - /api/... (другое) → 400 Bad Request
     *    - всё остальное → статический файл
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
            send(MakeBadRequestResponse(req));
        }
        // Статические файлы
        else {
            HandleStaticFile(req, std::forward<Send>(send), target);
        }
    }

private:
    model::Game& game_;                    ///< Ссылка на модель игры
    std::filesystem::path static_files_root_;  ///< Корень статики (например, ./static)

    // === КЭШИРОВАНИЕ ===
    mutable std::unordered_map<std::string, std::string> map_json_cache_;
    mutable std::string maps_list_json_;
    mutable bool maps_list_cache_valid_ = false;

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
        const http::request<Body>& req);

    // === API Обработчики ===

    /**
     * @brief Обрабатывает часть пути после /api/v1/maps
     * @param req Исходный запрос
     * @param send Callback для отправки ответа
     * @param path_suffix Часть пути после "api/v1/maps" (например, "/map1")
     */
    void HandleApiMaps(
        http::request<http::string_body>&& req,
        std::function<void(http::response<http::string_body>&&)>&& send,
        std::string_view path_suffix);

    /// Обрабатывает GET /api/v1/maps — возвращает список карт
    http::response<http::string_body> HandleMapsList(const http::request<http::string_body>& req);

    /// Обрабатывает GET /api/v1/maps/{id} — возвращает полное описание карты
    http::response<http::string_body> HandleMapById(const http::request<http::string_body>& req, std::string_view map_id);

    // === Статические файлы ===

    /// Обрабатывает запрос к статическому файлу (HTML, CSS, JS и т.д.)
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

    /// Создаёт ответ с ошибкой 400 Bad Request
    http::response<http::string_body> MakeBadRequestResponse(const http::request<http::string_body>& req);

    /// Создаёт ответ с ошибкой 404 Not Found
    http::response<http::string_body> MakeNotFoundResponse(const http::request<http::string_body>& req);

    /// Создаёт ответ с ошибкой 405 Method Not Allowed
    http::response<http::string_body> MakeMethodNotAllowedResponse(const http::request<http::string_body>& req);
};

}  // namespace http_handler