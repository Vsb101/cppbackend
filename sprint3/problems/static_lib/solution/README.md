# Генерация потерянных предметов на карте

Реализация генерации потерянных предметов для игрового сервера.

## Функциональность

1. **Генерация предметов (LootGenerator)**:
   - Предметы спавнятся на карте с течением времени
   - Использует параметры `period` и `probability` из конфигурации
   - Количество предметов не превышает количество собак (лоотеров)

2. **REST API**:
   - `/api/v1/game/state` - возвращает состояние игры, включая `lostObjects`
   - `/api/v1/maps/{id}` - возвращает информацию о карте с `lootTypes`

3. **Конфигурация**:
   - `lootGeneratorConfig` - настройки генератора
   - `lootTypes` - массив типов предметов для каждой карты

## Сборка

```bash
mkdir build && cd build
conan install .. --build=missing -s build_type=Release
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --target game_server --config Release
cmake --build . --target game_server_tests --config Release
```

## Запуск

```bash
./game_server --config-file ../data/config.json --www-root ../static --tick-period 1000
```

## Запуск тестов

```bash
./game_server_tests
```