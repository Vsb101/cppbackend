#pragma once

#include <memory>
#include <vector>
#include <string>
#include <utility>

#include "../other/tagged.h"
#include "dog.h"
#include "map.h"

namespace model {

class GameSession {
 public:
   using Id = util::Tagged<std::string, GameSession>;

   explicit GameSession(std::shared_ptr<Map> map);

   std::shared_ptr<Dog> CreateDog(const std::string& name);

   [[nodiscard]] const Id& GetId() const noexcept;
   [[nodiscard]] std::shared_ptr<Map> GetMap() const noexcept;
   [[nodiscard]] const std::vector<std::shared_ptr<Dog>>& GetDogs() const noexcept;

   // Обновление состояния за время dt
   void Update(double dt_seconds);

 private:
   std::shared_ptr<Map> map_;
   Id id_;
   std::vector<std::shared_ptr<Dog>> dogs_;
   size_t next_dog_id_ = 0; // Добавили поле счетчика
   
   // Находит дорогу, на которой находится собака
   const Road* FindRoadAndBounds(Position pos) const;

   // Находит все дороги в позиции (для перекрёстков)
   std::vector<const Road*> FindRoadsAtPosition(Position pos) const;
   
   // Проверяет, находится ли точка в пределах дороги
   bool IsPositionOnRoad(Position pos, const Road& road) const;

   void PutDogInRndPosition(std::shared_ptr<Dog> dog);
};

}  // namespace model
