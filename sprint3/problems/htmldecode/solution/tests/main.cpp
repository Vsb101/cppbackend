#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>

// Реализация StringMaker для std::string и std::string_view, т.к. в скомпилированной библиотеке Catch2 их нет
namespace Catch {
    std::string StringMaker<std::string>::convert(const std::string& str) {
        return '"' + str + '"';
    }

    std::string StringMaker<std::string_view>::convert(std::string_view str) {
        return '"' + std::string(str) + '"';
    }

    void formatReconstructedExpression(std::ostream &os, std::string const& lhs, StringRef op, std::string const& rhs) {
        os << lhs << ' ' << op << ' ' << rhs;
    }
}