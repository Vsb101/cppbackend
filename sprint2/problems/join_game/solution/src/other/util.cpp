// util.cpp
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

namespace util {

std::vector<std::string_view> SplitStr(std::string_view str) {
    std::vector<std::string_view> result;

    if (str.empty() || str == "/") { return result; }
    if (str.front() == '/')        { str.remove_prefix(1); }

    boost::split(result, str, boost::is_any_of("/"));
    
    return result;
}

std::string GetBearerToken(std::string_view bearer_string) {
    constexpr std::string_view kBearerPrefix = "Bearer ";
    constexpr size_t kTokenLength = 32;
    
    if (!bearer_string.starts_with(kBearerPrefix)) { return {}; }

    bearer_string.remove_prefix(kBearerPrefix.length());

    return (bearer_string.length() == kTokenLength) 
        ? std::string(bearer_string) 
        : std::string{};
}

}  // namespace util 