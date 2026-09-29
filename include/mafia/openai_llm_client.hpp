#pragma once

#include <string>
#include <string_view>

#include "mafia/llm_client.hpp"

namespace mafia {

struct OpenAiClientConfig {
    std::string baseUrl;
    std::string model;
    std::string apiKey;
    int timeoutSeconds = 20;
};

class OpenAiLlmClient final : public LlmClient {
public:
    explicit OpenAiLlmClient(OpenAiClientConfig config);
    std::string complete(std::string_view prompt) override;

private:
    OpenAiClientConfig config_;
};

}  // namespace mafia
