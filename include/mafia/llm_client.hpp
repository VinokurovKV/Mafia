#pragma once

#include <string>
#include <string_view>

namespace mafia {

class LlmClient {
public:
    virtual ~LlmClient() = default;
    virtual std::string complete(std::string_view prompt) = 0;
};

}  // namespace mafia
