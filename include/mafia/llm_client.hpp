#pragma once

#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace mafia {

class LlmClient {
public:
    virtual ~LlmClient() = default;
    virtual std::string complete(std::string_view prompt) = 0;

    virtual void startCompletion(std::string prompt) {
        if (pendingCompletion_.has_value()) {
            throw std::logic_error("LLM client already has a pending request");
        }
        pendingCompletion_ = complete(prompt);
    }

    virtual bool completionReady() const noexcept {
        return pendingCompletion_.has_value();
    }

    virtual std::string takeCompletion() {
        if (!pendingCompletion_.has_value()) {
            throw std::logic_error("LLM completion is not ready");
        }
        std::string result = std::move(*pendingCompletion_);
        pendingCompletion_.reset();
        return result;
    }

private:
    std::optional<std::string> pendingCompletion_;
};

}  // namespace mafia
