#include "menu.h"

#include <iomanip>
#include <sstream>

namespace menu {

struct OutputFlagsGuard {
    std::ostream& out;
    std::ios_base::fmtflags flags;
    char fill;
    
    ~OutputFlagsGuard() {
        out.flags(flags);
        out.fill(fill);
    }
};

Menu::Menu(std::istream& input, std::ostream& output)
    : input_{input}
    , output_{output} {
}

void Menu::AddAction(std::string action_name, std::string args, std::string description,
                     Handler handler) {
    const bool inserted = actions_.try_emplace(
        action_name, std::move(handler), std::move(args), std::move(description)
    ).second;
    
    if (!inserted) {
        throw std::invalid_argument("A command has been added already");
    }
    
    actions_width_ = std::max(actions_width_, action_name.length());
    args_width_ = std::max(args_width_, args.length());
}

void Menu::Run() {
    std::string line;
    while (std::getline(input_, line)) {
        std::istringstream cmd_stream{std::move(line)};
        if (!ParseCommand(cmd_stream)) {
            break;
        }
    }
}

void Menu::ShowInstructions() const {
    if (actions_.empty()) {
        return;
    }

    OutputFlagsGuard guard{output_, output_.flags(), output_.fill()};
    output_ << std::left << std::setfill(' ');
    
    for (const auto& [action_name, info] : actions_) {
        output_ << std::setw(actions_width_ + 1) << action_name;
        output_ << std::setw(args_width_ + 1) << info.args;
        output_ << info.description << std::endl;
    }
}

bool Menu::ParseCommand(std::istream& input) {
    using namespace std::literals;

    try {
        std::string cmd;
        if (input >> cmd) {
            if (const auto it = actions_.find(cmd); it != actions_.cend()) {
                if (!it->second.handler(input)) {
                    return false;
                }
            } else {
                output_ << "Command '"sv << cmd << "' has not been found."sv << std::endl;
            }
        } else {
            output_ << "Invalid command"sv << std::endl;
        }
    } catch (const std::exception& e) {
        output_ << e.what() << std::endl;
    }
    return true;
}

}  // namespace menu
