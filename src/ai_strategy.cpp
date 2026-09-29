#include "mafia/ai_strategy.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <limits>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace mafia {
namespace {

std::string_view roleName(RoleType role) {
    switch (role) {
        case RoleType::Mafia: return "Mafia";
        case RoleType::Civilian: return "Civilian";
        case RoleType::Doctor: return "Doctor";
        case RoleType::Commissioner: return "Commissioner";
        case RoleType::Maniac: return "Maniac";
        case RoleType::Eavesdropper: return "Eavesdropper";
        case RoleType::Witness: return "Witness";
        case RoleType::Bull: return "Bull";
    }
    return "Unknown";
}

void appendUtf8(std::string& output, unsigned int codePoint) {
    if (codePoint <= 0x7F) {
        output.push_back(static_cast<char>(codePoint));
    } else if (codePoint <= 0x7FF) {
        output.push_back(static_cast<char>(0xC0 | (codePoint >> 6)));
        output.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    } else if (codePoint <= 0xFFFF) {
        output.push_back(static_cast<char>(0xE0 | (codePoint >> 12)));
        output.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    } else {
        output.push_back(static_cast<char>(0xF0 | (codePoint >> 18)));
        output.push_back(static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    }
}

class JsonCursor {
public:
    explicit JsonCursor(std::string_view input) : input_(input) {}

    void skipWhitespace() {
        while (
            position_ < input_.size() &&
            std::isspace(static_cast<unsigned char>(input_[position_]))
        ) {
            ++position_;
        }
    }

    bool consume(char expected) {
        skipWhitespace();
        if (position_ >= input_.size() || input_[position_] != expected) {
            return false;
        }
        ++position_;
        return true;
    }

    bool finished() {
        skipWhitespace();
        return position_ == input_.size();
    }

    std::string parseString() {
        skipWhitespace();
        if (position_ >= input_.size() || input_[position_++] != '"') {
            throw std::invalid_argument("Expected a JSON string");
        }

        std::string result;
        while (position_ < input_.size()) {
            const char character = input_[position_++];
            if (character == '"') {
                return result;
            }
            if (character != '\\') {
                result.push_back(character);
                continue;
            }
            if (position_ >= input_.size()) {
                throw std::invalid_argument("Incomplete JSON escape");
            }
            const char escaped = input_[position_++];
            switch (escaped) {
                case '"': result.push_back('"'); break;
                case '\\': result.push_back('\\'); break;
                case '/': result.push_back('/'); break;
                case 'b': result.push_back('\b'); break;
                case 'f': result.push_back('\f'); break;
                case 'n': result.push_back('\n'); break;
                case 'r': result.push_back('\r'); break;
                case 't': result.push_back('\t'); break;
                case 'u': appendUtf8(result, parseHexCodePoint()); break;
                default:
                    throw std::invalid_argument("Unsupported JSON escape");
            }
        }
        throw std::invalid_argument("Unterminated JSON string");
    }

    PlayerId parsePlayerId() {
        skipWhitespace();
        const char* begin = input_.data() + position_;
        const char* end = input_.data() + input_.size();
        PlayerId value = 0;
        const auto [parsedEnd, error] = std::from_chars(begin, end, value);
        if (error != std::errc{} || parsedEnd == begin) {
            throw std::invalid_argument("Expected a JSON integer target");
        }
        position_ += static_cast<std::size_t>(parsedEnd - begin);
        return value;
    }

private:
    unsigned int parseHexCodePoint() {
        if (input_.size() - position_ < 4) {
            throw std::invalid_argument("Incomplete JSON unicode escape");
        }
        unsigned int value = 0;
        const char* begin = input_.data() + position_;
        const char* end = begin + 4;
        const auto [parsedEnd, error] = std::from_chars(begin, end, value, 16);
        if (error != std::errc{} || parsedEnd != end) {
            throw std::invalid_argument("Invalid JSON unicode escape");
        }
        position_ += 4;
        return value;
    }

    std::string_view input_;
    std::size_t position_ = 0;
};

std::string_view jsonObject(std::string_view response) {
    const std::size_t begin = response.find('{');
    const std::size_t end = response.rfind('}');
    if (begin == std::string_view::npos || end == std::string_view::npos || end < begin) {
        throw std::invalid_argument("LLM response does not contain a JSON object");
    }
    return response.substr(begin, end - begin + 1);
}

}  // namespace

AiRequestBudget::AiRequestBudget(std::size_t maximumPerRound)
    : maximumPerRound_(maximumPerRound) {
    if (maximumPerRound == 0) {
        throw std::invalid_argument("AI request budget must be positive");
    }
}

bool AiRequestBudget::tryConsume(int round) noexcept {
    if (currentRound_ != round) {
        currentRound_ = round;
        used_ = 0;
    }
    if (used_ >= maximumPerRound_) {
        return false;
    }
    ++used_;
    return true;
}

std::size_t AiRequestBudget::used() const noexcept {
    return used_;
}

AiStrategy::AiStrategy(
    SharedPtr<LlmClient> client,
    SharedPtr<DecisionStrategy> fallback,
    SharedPtr<AiRequestBudget> budget,
    std::string personality
)
    : client_(std::move(client)),
      fallback_(std::move(fallback)),
      budget_(std::move(budget)),
      personality_(std::move(personality)) {
    if (!client_ || !fallback_ || !budget_) {
        throw std::invalid_argument("AiStrategy dependencies must not be null");
    }
}

StrategyDecision AiStrategy::decide(const TurnContext& context) {
    if (context.phase != GamePhase::Voting) {
        return fallback_->decide(context);
    }
    if (!budget_->tryConsume(context.agent.round)) {
        return fallbackDecision(context, "request limit reached");
    }

    try {
        StrategyDecision decision = parseResponse(
            client_->complete(buildPrompt(context))
        );
        if (!isValid(decision, context)) {
            return fallbackDecision(context, "invalid model decision");
        }
        return decision;
    } catch (const std::exception& error) {
        return fallbackDecision(context, error.what());
    } catch (...) {
        return fallbackDecision(context, "unknown LLM failure");
    }
}

PlayerId AiStrategy::chooseTarget(const TurnContext& context) {
    return fallback_->chooseTarget(context);
}

ActionType AiStrategy::chooseActionType(const TurnContext& context) {
    return fallback_->chooseActionType(context);
}

std::string AiStrategy::buildPrompt(const TurnContext& context) const {
    const AgentContext& agent = context.agent;
    std::ostringstream prompt;
    prompt
        << "You are a player in a Mafia game. Make one daytime statement "
        << "and choose a voting target.\n"
        << "Round: " << agent.round << "\n"
        << "Your player id: " << agent.selfId << "\n"
        << "Your role: " << roleName(agent.role) << "\n"
        << "Personality: "
        << (agent.personality.empty() ? personality_ : agent.personality)
        << "\nLiving players:";
    for (const PlayerId id : agent.livingPlayers) {
        prompt << ' ' << id;
    }
    prompt << "\nValid voting targets:";
    for (const PlayerId id : context.availableTargets) {
        prompt << ' ' << id;
    }
    prompt << "\nPublic history:\n";
    if (agent.publicHistory.empty()) {
        prompt << "- No previous public events.\n";
    } else {
        for (const std::string& event : agent.publicHistory) {
            prompt << "- " << event << '\n';
        }
    }
    prompt << "Private knowledge:\n";
    if (agent.privateKnowledge.empty()) {
        prompt << "- None.\n";
    } else {
        for (const std::string& fact : agent.privateKnowledge) {
            prompt << "- " << fact << '\n';
        }
    }
    prompt
        << "Return only one JSON object with exactly these fields: "
        << "{\"action\":\"vote\",\"message\":\"...\","
        << "\"target\":NUMBER,\"reasoning\":\"...\"}. "
        << "The target must be one of the valid voting targets. "
        << "Do not reveal your role or private knowledge in the message.";
    return prompt.str();
}

StrategyDecision AiStrategy::parseResponse(std::string_view response) const {
    JsonCursor cursor(jsonObject(response));
    if (!cursor.consume('{')) {
        throw std::invalid_argument("Expected a JSON object");
    }

    StrategyDecision decision;
    bool hasAction = false;
    bool hasMessage = false;
    bool hasTarget = false;
    bool hasReasoning = false;
    bool first = true;
    while (!cursor.consume('}')) {
        if (!first && !cursor.consume(',')) {
            throw std::invalid_argument("Expected a comma in JSON object");
        }
        first = false;
        const std::string key = cursor.parseString();
        if (!cursor.consume(':')) {
            throw std::invalid_argument("Expected a colon in JSON object");
        }
        if (key == "action") {
            if (hasAction) {
                throw std::invalid_argument("Duplicate action field");
            }
            if (cursor.parseString() != "vote") {
                throw std::invalid_argument("AI action must be vote");
            }
            decision.action = ActionType::Vote;
            hasAction = true;
        } else if (key == "message") {
            if (hasMessage) {
                throw std::invalid_argument("Duplicate message field");
            }
            decision.message = cursor.parseString();
            hasMessage = true;
        } else if (key == "target") {
            if (hasTarget) {
                throw std::invalid_argument("Duplicate target field");
            }
            decision.target = cursor.parsePlayerId();
            hasTarget = true;
        } else if (key == "reasoning") {
            if (hasReasoning) {
                throw std::invalid_argument("Duplicate reasoning field");
            }
            decision.reasoning = cursor.parseString();
            hasReasoning = true;
        } else {
            throw std::invalid_argument("Unexpected field in AI response");
        }
    }
    if (!cursor.finished() || !hasAction || !hasMessage || !hasTarget || !hasReasoning) {
        throw std::invalid_argument("AI response is missing required fields");
    }
    return decision;
}

bool AiStrategy::isValid(
    const StrategyDecision& decision,
    const TurnContext& context
) const noexcept {
    return decision.action == ActionType::Vote &&
        std::ranges::find(context.availableTargets, decision.target) !=
            context.availableTargets.end() &&
        !decision.message.empty() && decision.message.size() <= 500 &&
        !decision.reasoning.empty() && decision.reasoning.size() <= 1000;
}

StrategyDecision AiStrategy::fallbackDecision(
    const TurnContext& context,
    std::string reason
) {
    StrategyDecision decision = fallback_->decide(context);
    if (decision.message.empty()) {
        decision.message = "I do not have enough reliable information yet.";
    }
    decision.reasoning = "AI fallback: " + std::move(reason);
    return decision;
}

}  // namespace mafia
