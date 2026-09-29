#include "mafia/console_strategy.hpp"

#include <algorithm>
#include <charconv>
#include <istream>
#include <iostream>
#include <ostream>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/select.h>
#include <unistd.h>
#endif

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

void ConsoleStrategy::startDecision(const TurnContext& context) {
    if (pendingContext_.has_value() || inputStage_ != InputStage::None) {
        throw std::logic_error(
            "ConsoleStrategy already has a pending decision"
        );
    }
    if (context.availableActions.empty() || context.availableTargets.empty()) {
        throw std::logic_error("No console decision is available");
    }

    pendingContext_ = context;
    inputClosed_ = false;
    selectedAction_ = context.availableActions.front();
    if (context.availableActions.size() == 1) {
        selectedAction_ = context.availableActions.front();
        promptForTarget();
        return;
    }

    output_ << "Available actions:\n";
    for (std::size_t index = 0; index < context.availableActions.size(); ++index) {
        output_
            << "  " << index + 1 << ") "
            << actionName(context.availableActions[index]) << "\n";
    }
    output_ << "Choose action by number: " << std::flush;
    inputStage_ = InputStage::Action;
}

bool ConsoleStrategy::decisionReady() {
    if (inputStage_ == InputStage::Ready) {
        return true;
    }
    if (!pendingContext_.has_value() || inputStage_ == InputStage::None) {
        throw std::logic_error("ConsoleStrategy has no pending decision");
    }

    const std::optional<std::string> line = readAvailableLine();
    if (!line.has_value()) {
        return false;
    }
    if (inputClosed_) {
        useEndOfInputFallback();
        return true;
    }

    std::size_t selected = 0;
    if (inputStage_ == InputStage::Action) {
        if (
            parsePlayerId(*line, selected) && selected > 0 &&
            selected <= pendingContext_->availableActions.size()
        ) {
            selectedAction_ = pendingContext_->availableActions[selected - 1];
            promptForTarget();
        } else {
            output_ << "Invalid action. Enter one of the listed numbers: "
                    << std::flush;
        }
        return false;
    }

    if (
        parsePlayerId(*line, selected) &&
        std::ranges::find(
            pendingContext_->availableTargets,
            selected
        ) != pendingContext_->availableTargets.end()
    ) {
        pendingDecision_ = StrategyDecision{
            selectedAction_,
            selected,
            {},
            {},
        };
        inputStage_ = InputStage::Ready;
        return true;
    }
    output_ << "Invalid target. Enter one of the listed ids: " << std::flush;
    return false;
}

StrategyDecision ConsoleStrategy::takeDecision() {
    if (!pendingDecision_.has_value()) {
        throw std::logic_error("Console decision is not ready");
    }
    StrategyDecision decision = std::move(*pendingDecision_);
    pendingDecision_.reset();
    pendingContext_.reset();
    inputStage_ = InputStage::None;
    inputClosed_ = false;
    return decision;
}

void ConsoleStrategy::promptForTarget() {
    if (pendingContext_->availableTargets.size() == 1) {
        output_
            << "Only available target: Player #"
            << pendingContext_->availableTargets.front() << "\n";
        pendingDecision_ = StrategyDecision{
            selectedAction_,
            pendingContext_->availableTargets.front(),
            {},
            {},
        };
        inputStage_ = InputStage::Ready;
        return;
    }

    output_ << "Available targets:";
    for (const PlayerId target : pendingContext_->availableTargets) {
        output_ << " #" << target;
    }
    output_ << "\nChoose target by id: " << std::flush;
    inputStage_ = InputStage::Target;
}

std::optional<std::string> ConsoleStrategy::readAvailableLine() {
    bool inputReady = input_.rdbuf()->in_avail() > 0 || input_.eof();
    if (&input_ == &std::cin && !inputReady) {
#ifdef _WIN32
        const HANDLE handle = GetStdHandle(STD_INPUT_HANDLE);
        if (handle != nullptr && handle != INVALID_HANDLE_VALUE) {
            const DWORD fileType = GetFileType(handle);
            if (fileType == FILE_TYPE_CHAR) {
                DWORD eventCount = 0;
                if (GetNumberOfConsoleInputEvents(handle, &eventCount) &&
                    eventCount > 0) {
                    std::vector<INPUT_RECORD> events(eventCount);
                    DWORD read = 0;
                    if (PeekConsoleInputW(
                            handle, events.data(), eventCount, &read
                        )) {
                        inputReady = std::ranges::any_of(
                            events.begin(),
                            events.begin() + read,
                            [](const INPUT_RECORD& event) {
                                return event.EventType == KEY_EVENT &&
                                    event.Event.KeyEvent.bKeyDown &&
                                    event.Event.KeyEvent.wVirtualKeyCode ==
                                        VK_RETURN;
                            }
                        );
                    }
                }
            } else {
                inputReady = WaitForSingleObject(handle, 0) == WAIT_OBJECT_0;
            }
        }
#else
        fd_set descriptors;
        FD_ZERO(&descriptors);
        FD_SET(STDIN_FILENO, &descriptors);
        timeval timeout{};
        inputReady = select(
            STDIN_FILENO + 1,
            &descriptors,
            nullptr,
            nullptr,
            &timeout
        ) > 0;
#endif
    }
    if (!inputReady) {
        return std::nullopt;
    }

    std::string line;
    if (!std::getline(input_, line)) {
        inputClosed_ = true;
    }
    return line;
}

void ConsoleStrategy::useEndOfInputFallback() {
    const PlayerId target = pendingContext_->availableTargets.front();
    output_
        << "\nInput closed. Selecting Player #" << target << ".\n";
    pendingDecision_ = StrategyDecision{
        selectedAction_,
        target,
        {},
        {},
    };
    inputStage_ = InputStage::Ready;
}

}  // namespace mafia
