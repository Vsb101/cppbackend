#pragma once
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <string>

#include "tagged.h"

namespace util {

namespace detail {

using UUIDType = boost::uuids::uuid;

inline UUIDType NewUUID() {
    return boost::uuids::random_generator()();
}

constexpr UUIDType ZeroUUID{{0}};

inline std::string UUIDToString(const UUIDType& uuid) {
    return boost::uuids::to_string(uuid);
}

inline UUIDType UUIDFromString(std::string_view str) {
    boost::uuids::string_generator gen;
    return gen(str.begin(), str.end());
}

}  // namespace detail

template <typename Tag>
class TaggedUUID : public Tagged<detail::UUIDType, Tag> {
public:
    using Base = Tagged<detail::UUIDType, Tag>;
    using Tagged<detail::UUIDType, Tag>::Tagged;

    TaggedUUID()
        : Base{detail::ZeroUUID} {
    }

    static TaggedUUID New() {
        return TaggedUUID{detail::NewUUID()};
    }

    static TaggedUUID FromString(const std::string& uuid_as_text) {
        return TaggedUUID{detail::UUIDFromString(uuid_as_text)};
    }


    std::string ToString() const {
        return detail::UUIDToString(**this);
    }
};

}  // namespace util