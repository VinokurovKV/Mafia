#pragma once

#include <memory>
#include <string>
#include <string_view>

#include "mafia/llm_client.hpp"

namespace mafia {

struct OpenAiAsyncState;

struct OpenAiClientConfig {
    std::string baseUrl;
    std::string model;
    std::string apiKey;
    int timeoutSeconds = 20;
};

class OpenAiLlmClient final : public LlmClient {
public:
    explicit OpenAiLlmClient(OpenAiClientConfig config);
    ~OpenAiLlmClient() override;
    std::string complete(std::string_view prompt) override;
    void startCompletion(std::string prompt) override;
    bool completionReady() const noexcept override;
    std::string takeCompletion() override;

private:
    OpenAiClientConfig config_;
    std::unique_ptr<OpenAiAsyncState> asyncState_;
};

}  // namespace mafia
