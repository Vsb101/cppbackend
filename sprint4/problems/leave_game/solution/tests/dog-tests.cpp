#include <catch2/catch_test_macros.hpp>
#include "../src/model/dog.h"

SCENARIO("Dog bag management") {
    GIVEN("a new dog with bag capacity 3") {
        model::Dog dog{model::Dog::Id{1}, "TestDog", 3};
        
        THEN("bag is initially empty") {
            REQUIRE(dog.GetBag().empty());
            REQUIRE_FALSE(dog.IsBagFull());
        }
        
        WHEN("adding items to bag") {
            dog.AddToBag(1, 10);
            dog.AddToBag(2, 10);
            dog.AddToBag(3, 11);
            
            THEN("bag contains 3 items") {
                REQUIRE(dog.GetBag().size() == 3);
                REQUIRE(dog.IsBagFull());
                
                const auto& bag = dog.GetBag();
                CHECK(bag[0].id == 1);
                CHECK(bag[0].type == 10);
                CHECK(bag[1].id == 2);
                CHECK(bag[1].type == 10);
                CHECK(bag[2].id == 3);
                CHECK(bag[2].type == 11);
            }
            
            WHEN("trying to add more items when bag is full") {
                bool result = dog.AddToBag(4, 12);
                
                THEN("item is not added") {
                    CHECK_FALSE(result);
                    REQUIRE(dog.GetBag().size() == 3);
                }
            }
            
            WHEN("clearing bag at base") {
                auto cleared = dog.ClearBag();
                
                THEN("bag is cleared and items returned") {
                    REQUIRE(cleared.size() == 3);
                    REQUIRE(dog.GetBag().empty());
                    REQUIRE_FALSE(dog.IsBagFull());
                }
            }
        }
    }
    
    GIVEN("a dog with default bag capacity") {
        model::Dog dog{model::Dog::Id{2}, "DefaultDog"};
        
        THEN("default capacity is 3") {
            REQUIRE(dog.GetBagCapacity() == 3);
        }
    }
}

SCENARIO("Dog score management") {
    GIVEN("a new dog") {
        model::Dog dog{model::Dog::Id{1}, "TestDog", 3};

        THEN("score is initially zero") {
            REQUIRE(dog.GetScore() == 0);
        }

        WHEN("adding score points") {
            dog.AddScore(10);
            dog.AddScore(25);

            THEN("score accumulates") {
                REQUIRE(dog.GetScore() == 35);
            }
        }
    }

    GIVEN("item id and type") {
        uint32_t id = 42;
        size_t type = 5;
        
        WHEN("creating BagItem") {
            model::Dog::BagItem item{id, type};
            
            THEN("item has correct values") {
                CHECK(item.id == 42);
                CHECK(item.type == 5);
            }
        }
    }
}
