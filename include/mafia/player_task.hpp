#pragma once

#include <coroutine>
#include <exception>
#include <optional>
#include <stdexcept>
#include <utility>

#include "mafia/types.hpp"

namespace mafia {

class PlayerTask {
public:
    struct promise_type {
        PlayerTask get_return_object() noexcept {
            return PlayerTask{
                std::coroutine_handle<promise_type>::from_promise(*this)
            };
        }

        std::suspend_always initial_suspend() const noexcept {
            return {};
        }

        std::suspend_always final_suspend() const noexcept {
            return {};
        }

        std::suspend_always yield_value(Action action) noexcept {
            yieldedAction = std::move(action);
            return {};
        }

        void return_void() const noexcept {}

        void unhandled_exception() noexcept {
            exception = std::current_exception();
        }

        std::optional<Action> yieldedAction;
        std::exception_ptr exception;
    };

    PlayerTask() noexcept = default;

    PlayerTask(const PlayerTask&) = delete;
    PlayerTask& operator=(const PlayerTask&) = delete;

    PlayerTask(PlayerTask&& other) noexcept
        : handle_(std::exchange(other.handle_, {})) {}

    PlayerTask& operator=(PlayerTask&& other) noexcept {
        if (this != &other) {
            if (handle_) {
                handle_.destroy();
            }
            handle_ = std::exchange(other.handle_, {});
        }
        return *this;
    }

    ~PlayerTask() {
        if (handle_) {
            handle_.destroy();
        }
    }

    void resume() {
        if (!handle_ || handle_.done()) {
            throw std::logic_error("Player coroutine cannot be resumed");
        }

        handle_.resume();
        if (handle_.promise().exception) {
            std::rethrow_exception(handle_.promise().exception);
        }
    }

    bool hasAction() const noexcept {
        return handle_ && handle_.promise().yieldedAction.has_value();
    }

    Action takeAction() {
        if (!handle_ || !handle_.promise().yieldedAction.has_value()) {
            throw std::logic_error("Player coroutine did not yield an action");
        }

        Action action = std::move(*handle_.promise().yieldedAction);
        handle_.promise().yieldedAction.reset();
        return action;
    }

private:
    explicit PlayerTask(std::coroutine_handle<promise_type> handle) noexcept
        : handle_(handle) {}

    std::coroutine_handle<promise_type> handle_{};
};

}  // namespace mafia
