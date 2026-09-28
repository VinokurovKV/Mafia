#include "mafia/role_config.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>

namespace mafia {
namespace {

std::string_view trim(std::string_view value) {
    const auto isSpace = [](unsigned char character) {
        return std::isspace(character) != 0;
    };
    while (!value.empty() && isSpace(value.front())) {
        value.remove_prefix(1);
    }
    while (!value.empty() && isSpace(value.back())) {
        value.remove_suffix(1);
    }
    return value;
}

RoleType parseRole(std::string_view name, std::size_t lineNumber) {
    if (name == "eavesdropper") {
        return RoleType::Eavesdropper;
    }
    if (name == "witness") {
        return RoleType::Witness;
    }
    if (name == "bull") {
        return RoleType::Bull;
    }
    throw std::invalid_argument(
        "Unknown additional role '" + std::string(name) +
        "' at line " + std::to_string(lineNumber)
    );
}

}  // namespace

std::vector<RoleType> loadAdditionalRoles(
    const std::filesystem::path& path
) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error(
            "Cannot open role configuration: " + path.string()
        );
    }

    std::vector<RoleType> roles;
    bool sectionFound = false;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        const std::size_t comment = line.find('#');
        if (comment != std::string::npos) {
            line.erase(comment);
        }
        const std::string_view value = trim(line);
        if (value.empty() || value == "---") {
            continue;
        }
        if (value == "additional_roles:") {
            if (sectionFound) {
                throw std::invalid_argument(
                    "Duplicate additional_roles section at line " +
                    std::to_string(lineNumber)
                );
            }
            sectionFound = true;
            continue;
        }
        if (!sectionFound || !value.starts_with('-')) {
            throw std::invalid_argument(
                "Expected an additional role list item at line " +
                std::to_string(lineNumber)
            );
        }

        const RoleType role = parseRole(trim(value.substr(1)), lineNumber);
        if (std::ranges::find(roles, role) != roles.end()) {
            throw std::invalid_argument(
                "Additional role is listed more than once at line " +
                std::to_string(lineNumber)
            );
        }
        roles.push_back(role);
    }

    if (!sectionFound) {
        throw std::invalid_argument(
            "Role configuration must contain additional_roles"
        );
    }
    return roles;
}

}  // namespace mafia
