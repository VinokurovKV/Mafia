#include "mafia/console_strategy.hpp"

#include <algorithm>
#include <charconv>
#include <istream>
#include <ostream>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>

namespace mafia {
namespace {

std::string_view actionName(ActionType action) {
    switch (action) {
        case ActionType::Vote:
            return "Vote";
        case ActionType::MafiaKill:
            return "Mafia kill";
        case ActionType::Heal:
            return "Heal";
        case ActionType::Check:
            return "Check";
        case ActionType::Shoot:
            return "Shoot";
        case ActionType::ManiacKill:
            return "Maniac kill";
        case ActionType::Listen:
            return "Listen";
        case ActionType::Observe:
            return "Observe";
    }
    return "Unknown";
}

bool parsePlayerId(std::string_view text, PlayerId& value) {
    if (text.empty()) {
        return false;
    }

    const auto [end, error] = std::from_chars(
        text.data(),
        text.data() + text.size(),
        value
    );
    return error == std::errc{} && end == text.data() + text.size();
}

}  // namespace

ConsoleStrategy::ConsoleStrategy(std::istream& input, std::ostream& output)
    : input_(input),
      output_(output) {}

bool ConsoleStrategy::isInteractive() const noexcept {
    return true;
}

PlayerId ConsoleStrategy::chooseTarget(const TurnContext& context) {
    if (context.availableTargets.empty()) {
        throw std::logic_error("No targets are available");
    }

    if (context.availableTargets.size() == 1) {
        output_
            << "Only available target: Player #"
            << context.availableTargets.front() << "\n";
        return context.availableTargets.front();
    }

    output_ << "Available targets:";
    for (const PlayerId target : context.availableTargets) {
        output_ << " #" << target;
    }
    output_ << "\nChoose target by id: " << std::flush;

    std::string line;
    while (std::getline(input_, line)) {
        PlayerId selected = 0;
        if (
            parsePlayerId(line, selected) &&
            std::ranges::find(context.availableTargets, selected) !=
                context.availableTargets.end()
        ) {
            return selected;
        }
        output_ << "Invalid target. Enter one of the listed ids: "
                << std::flush;
    }

    const PlayerId fallback = context.availableTargets.front();
    output_
        << "\nInput closed. Selecting Player #"
        << fallback << ".\n";
    return fallback;
}

ActionType ConsoleStrategy::chooseActionType(const TurnContext& context) {
    if (context.availableActions.empty()) {
        throw std::logic_error("No action types are available");
    }
    if (context.availableActions.size() == 1) {
        return context.availableActions.front();
    }

    output_ << "Available actions:\n";
    for (std::size_t index = 0; index < context.availableActions.size(); ++index) {
        output_
            << "  " << index + 1 << ") "
            << actionName(context.availableActions[index]) << "\n";
    }
    output_ << "Choose action by number: " << std::flush;

    std::string line;
    while (std::getline(input_, line)) {
        std::size_t selected = 0;
        if (
            parsePlayerId(line, selected) &&
            selected > 0 &&
            selected <= context.availableActions.size()
        ) {
            return context.availableActions[selected - 1];
        }
        output_ << "Invalid action. Enter one of the listed numbers: "
                << std::flush;
    }

    const ActionType fallback = context.availableActions.front();
    output_
        << "\nInput closed. Selecting "
        << actionName(fallback) << ".\n";
    return fallback;
}

}  // namespace mafia
