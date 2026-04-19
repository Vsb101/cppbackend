#include "../other/file_system.h"

#include <system_error>

namespace util {

bool IsSubPath(const std::filesystem::path& path,
               const std::filesystem::path& base) {
    std::error_code ec;
    const auto rel = std::filesystem::relative(path, base, ec);
    
    // Если путь не выходит за пределы base, relative не начнётся с ".."
    return !ec && (!rel.has_relative_path() || !rel.root_path().empty());
}

}  // namespace util