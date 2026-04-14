#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "tagged.h"

namespace model {

// Предварительное объявление классов для разрешения циклических зависимостей
class Map;

/// Тип для коо��динат и размеров (целочисленные значения)
using Dimension = int;
/// Тип для координатной точки (alias для Dimension)
using Coord = Dimension;

//=============================================================================
// Базовые структуры
//=============================================================================

/// Точка на игровой карте (система координат X, Y)
struct Point {
    Coord x;  ///< Координата по горизонтали
    Coord y;  ///< Координата по вертикали
};

/// Размер объекта на карте (ширина и высота)
struct Size {
    Dimension width;   ///< Ширина
    Dimension height;  ///< Высота
};

/// Прямоугольная область на карте (положение + размер)
struct Rectangle {
    Point position;  ///< Координаты верхнего левого угла
    Size size;       ///< Размеры (ширина и высота)
};

/// Смещение относительно какой-то точки
struct Offset {
    Dimension dx;  ///< Смещение по оси X
    Dimension dy;  ///< Смещение по оси Y
};

//=============================================================================
// Road - дорога на карте
//=============================================================================

/**
 * @class Road
 * @brief Представляет дорогу на игровой карте.
 *
 * Дорога может быть горизонтальной или вертикальной.
 * Горизонтальная дорога: y0 == y1, меняется x
 * Вертикальная дорога: x0 == x1, меняется y
 *
 * Пример создания:
 * @code
 * // Горизонтальная дорога от (0,0) до (10,0)
 * Road horizontal(Road::HORIZONTAL, {0, 0}, 10);
 * // Вертикальная дорога от (0,0) до (0,5)
 * Road vertical(Road::VERTICAL, {0, 0}, 5);
 * @endcode
 */
class Road {
    /// Тег для создания горизонтальной дороги
    struct HorizontalTag {
        explicit HorizontalTag() = default;
    };

    /// Тег для создания вертикальной дороги
    struct VerticalTag {
        explicit VerticalTag() = default;
    };

public:
    /// Константа для создания горизонтальной дороги
    constexpr static HorizontalTag HORIZONTAL{};
    /// Константа для создания вертикальной дороги
    constexpr static VerticalTag VERTICAL{};

    /**
     * @brief Конструктор горизонтальной дороги
     * @param start Начальная точка дороги
     * @param end_x Координата X конечной точки
     */
    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {
    }

    /**
     * @brief Конструктор вертикальной дороги
     * @param start Начальная точка дороги
     * @param end_y Координата Y конечной точки
     */
    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
    }

    /// @return true если дорога горизонтальная (y0 == y1)
    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    /// @return true если дорога вертикальная (x0 == x1)
    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    /// @return Начальная точка дороги
    Point GetStart() const noexcept {
        return start_;
    }

    /// @return Конечная точка дороги
    Point GetEnd() const noexcept {
        return end_;
    }

private:
    Point start_;  ///< Начальная точка
    Point end_;    ///< Конечная точка
};

//=============================================================================
// Building - здание на карте
//=============================================================================

/**
 * @class Building
 * @brief Представляет здание на игровой карте.
 *
 * Здание задаётся прямоугольной областью (bounds).
 */
class Building {
public:
    /**
     * @brief Конструктор здания
     * @param bounds Прямоугольная область, занимаемая зданием
     */
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    /// @return Прямоугольная область здания
    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;  ///< Границы здания
};

//=============================================================================
// Office - офис на карте
//=============================================================================

/**
 * @class Office
 * @brief Представляет офис (например, склад) на игровой карте.
 *
 * Офис имеет уникальный идентификатор, позицию на карте
 * и смещение для визуализации.
 */
class Office {
public:
    /// Тип идентификатора офиса (строка с тегом)
    using Id = util::Tagged<std::string, Office>;

    /**
     * @brief Конструктор офиса
     * @param id Уникальный идентификатор
     * @param position Позиция на карте
     * @param offset Смещение для визуализации
     */
    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    /// @return Идентификатор офиса
    const Id& GetId() const noexcept {
        return id_;
    }

    /// @return Позиция офиса на карте
    Point GetPosition() const noexcept {
        return position_;
    }

    /// @return Смещение офиса
    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;          ///< Уникальный идентификатор
    Point position_; ///< Позиция на карте
    Offset offset_;  ///< Смещение для визуализации
};

//=============================================================================
// Dog - пёс на карте
//=============================================================================

/**
 * @class Dog
 * @brief Представляет пса (игровой объект) на карте.
 *
 * Пёс имеет имя (userName) и находится на определённой карте.
 */
class Dog {
public:
    /// Тип идентификатора пса (строка с тегом)
    using Id = util::Tagged<std::string, Dog>;

    /**
     * @brief Конструктор пса
     * @param name Имя пса (совпадает с именем игрока)
     * @param map Указатель на карту, где находится пёс
     */
    Dog(std::string name, const Map* map) noexcept
        : name_(std::move(name)), map_(map) {}

    /// @return Имя пса
    const std::string& GetName() const noexcept { return name_; }

    /// @return Указатель на карту, где находится пёс
    const Map* GetMap() const noexcept { return map_; }

private:
    std::string name_;       ///< Имя пса
    const Map* map_ = nullptr;  ///< Указатель на карту
};

//=============================================================================
// Player - игрок, управляющий псом
//=============================================================================

/**
 * @class Player
 * @brief Представляет игрока в игре.
 *
 * Игрок управляется пользователем через токен и имеет уникальный идентификатор.
 */
class Player {
public:
    /// Тип идентификатора игрока (целое число с тегом)
    using Id = util::Tagged<int, Player>;

    /// Тип токена аутентификации (строка)
    using Token = std::string;

    /**
     * @brief Конструктор игрока
     * @param id Уникальный идентификатор игрока
     * @param token Токен для аутентификации
     * @param dog Указатель на пса, которым управляет игрок
     */
    Player(Id id, Token token, Dog* dog) noexcept
        : id_(id), token_(std::move(token)), dog_(dog) {}

    /// @return Идентификатор игрока
    Id GetId() const noexcept { return id_; }

    /// @return Токен игрока
    const Token& GetToken() const noexcept { return token_; }

    /// @return Указатель на пса
    Dog* GetDog() const noexcept { return dog_; }

private:
    Id id_;                    ///< Идентификатор игрока
    Token token_;               ///< Токен аутентификации
    Dog* dog_ = nullptr;        ///< Указатель на пса
};

//=============================================================================
// GameSession - игровая сессия на одной карте
//=============================================================================

/**
 * @class GameSession
 * @brief Представляет игровую сессию на определённой карте.
 *
 * Содержит всех игроков и собак, находящихся на одной карте.
 */
class GameSession {
public:
    /**
     * @brief Конструктор сессии
     * @param map Указатель на карту сессии
     */
    explicit GameSession(const Map* map) noexcept : map_(map) {}

    /// @return Указатель на карту сессии
    const Map* GetMap() const noexcept { return map_; }

    /// @return Список всех игроков в сессии
    const std::vector<Player>& GetPlayers() const noexcept { return players_; }

    /**
     * @brief Добавляет игрока в сессию
     * @param player Игрока для добавления
     */
    void AddPlayer(Player player) {
        players_.emplace_back(std::move(player));
    }

private:
    const Map* map_ = nullptr;           ///< Указатель на карту
    std::vector<Player> players_;         ///< Список игроков
};

//=============================================================================
// Map - игровая карта
//=============================================================================

/**
 * @class Map
 * @brief Представляет игровую карту.
 *
 * Карта содержит дороги, здания и офисы.
 * Имеет уникальный идентификатор и название.
 */
class Map {
public:
    /// Тип идентификатора карты (строка с тегом)
    using Id = util::Tagged<std::string, Map>;
    /// Контейнер для хранения дорог
    using Roads = std::vector<Road>;
    /// Контейнер для хранения зданий
    using Buildings = std::vector<Building>;
    /// Контейнер для хранения офисов
    using Offices = std::vector<Office>;

    /**
     * @brief Конструктор карты
     * @param id Уникальный идентификатор карты
     * @param name Название карты
     */
    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    /// @return Идентификатор карты
    const Id& GetId() const noexcept {
        return id_;
    }

    /// @return Название карты
    const std::string& GetName() const noexcept {
        return name_;
    }

    /// @return Контейнер зданий
    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    /// @return Контейнер дорог
    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    /// @return Контейнер офисов
    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    /// @brief Добавляет дорогу на карту
    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    /// @brief Добавляет здание на карту
    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    /// @brief Добавляет офис на карту
    void AddOffice(Office office);

private:
    /// Карта для быстрого поиска офиса по id
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;                      ///< Идентификатор карты
    std::string name_;           ///< Название карты
    Roads roads_;                ///< Дороги на карте
    Buildings buildings_;        ///< Здания на карте
    OfficeIdToIndex warehouse_id_to_index_;  ///< Индекс офисов для быстрого поиска
    Offices offices_;            ///< Офисы на карте
};

//=============================================================================
// Game - игра (коллекция карт)
//=============================================================================

/**
 * @class Game
 * @brief Представляет игру - коллекцию карт и сессий.
 *
 * Является фасадом для доступа к картам и игровым сессиям.
 * Позволяет добавлять карты, искать их по идентификатору,
 * создавать сессии и управлять игроками.
 */
class Game {
public:
    /// Контейнер для хранения карт
    using Maps = std::vector<Map>;
    /// Контейнер для хранения игровых сессий
    using Sessions = std::vector<GameSession>;
    /// Контейнер для хранения всех собак
    using Dogs = std::vector<Dog>;
    /// Контейнер для хранения всех игроков
    using Players = std::vector<Player>;

    /**
     * @brief Добавляет карту в игру
     * @param map Карта для добавления
     * @throw std::invalid_argument если карта с таким id уже существует
     */
    void AddMap(Map map);

    /// @return Контейнер со всеми картами
    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    /**
     * @brief Ищет карту по идентификатору
     * @param id Идентификатор карты
     * @return Указатель на карту или nullptr, если не найдена
     */
    const Map* FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }

    /**
     * @brief Создаёт новую сессию на карте
     * @param map Указатель на карту для сессии
     * @return Ссылка на созданную сессию
     */
    GameSession& CreateSession(const Map* map) {
        sessions_.emplace_back(map);
        return sessions_.back();
    }

    /**
     * @brief Находит сессию по карте
     * @param map Указатель на карту
     * @return Указатель на сессию или nullptr, если не найдена
     */
    GameSession* FindSession(const Map* map) noexcept {
        auto it = std::find_if(sessions_.begin(), sessions_.end(),
                              [map](const GameSession& session) {
                                  return session.GetMap() == map;
                              });
        return it != sessions_.end() ? &(*it) : nullptr;
    }

    /**
     * @brief Создаёт нового пса
     * @param name Имя пса
     * @param map Указатель на карту, где появится пёс
     * @return Указатель на созданного пса
     */
    Dog* CreateDog(std::string name, const Map* map) {
        dogs_.emplace_back(std::move(name), map);
        return &dogs_.back();
    }

    /**
     * @brief Создаёт нового игрока
     * @param id Идентификатор игрока
     * @param token Токен аутентификации
     * @param dog Указатель на пса, которым управляет игрок
     * @return Указатель на созданного игрока
     */
    Player* CreatePlayer(Player::Id id, Player::Token token, Dog* dog) {
        players_.emplace_back(id, std::move(token), dog);
        return &players_.back();
    }

    /**
     * @brief Находит игрока по токену
     * @param token Токен игрока
     * @return Указатель на игрока или nullptr, если не найден
     */
    Player* FindPlayerByToken(const std::string& token) noexcept {
        auto it = std::find_if(players_.begin(), players_.end(),
                              [&token](const Player& player) {
                                  return player.GetToken() == token;
                              });
        return it != players_.end() ? &(*it) : nullptr;
    }

    /// @return Количество игроков
    size_t GetPlayersCount() const noexcept {
        return players_.size();
    }

private:
    /// Хешер для идентификатора карты
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    /// Карта для быстрого поиска карты по id
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::vector<Map> maps_;        ///< Все карты игры
    MapIdToIndex map_id_to_index_; ///< Индекс карт для быстрого поиска
    std::vector<GameSession> sessions_;  ///< Все игровые сессии
    std::vector<Dog> dogs_;              ///< Все собаки в игре
    std::vector<Player> players_;        ///< Все игроки в игре
};

}  // namespace model
