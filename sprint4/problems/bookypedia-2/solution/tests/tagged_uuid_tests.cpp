#include <catch2/catch_test_macros.hpp>

#include "../src/util/tagged_uuid.h"

using util::TaggedUUID;

namespace {
// Тег для тестового UUID
struct TestTag {};
using TestUUID = TaggedUUID<TestTag>;
}  // namespace

// Тест: преобразование UUID в строку и обратно
TEST_CASE("UUID-String conversion") {
    auto uuid = TestUUID::New();
    auto s = uuid.ToString();
    CHECK(TestUUID::FromString(s) == uuid);
}