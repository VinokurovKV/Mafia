#include "mafia/openai_llm_client.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#endif

namespace mafia {

struct OpenAiAsyncState {
#ifdef _WIN32
    HINTERNET session = nullptr;
    HINTERNET connection = nullptr;
    HINTERNET request = nullptr;
    std::array<char, 8192> buffer{};
    std::string requestBody;
    std::string responseBody;
    std::string error;
    DWORD status = 0;
    mutable std::mutex mutex;
    std::atomic_bool ready{false};

    ~OpenAiAsyncState() {
        if (request != nullptr) WinHttpCloseHandle(request);
        if (connection != nullptr) WinHttpCloseHandle(connection);
        if (session != nullptr) WinHttpCloseHandle(session);
    }
#else
    std::string error;
    std::atomic_bool ready{false};
#endif
};

namespace {

std::string escapeJson(std::string_view value) {
    std::string escaped;
    escaped.reserve(value.size() + 16);
    for (const unsigned char character : value) {
        switch (character) {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default:
                if (character < 0x20) {
                    constexpr char hex[] = "0123456789abcdef";
                    escaped += "\\u00";
                    escaped.push_back(hex[character >> 4]);
                    escaped.push_back(hex[character & 0x0F]);
                } else {
                    escaped.push_back(static_cast<char>(character));
                }
        }
    }
    return escaped;
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

unsigned int parseHex(std::string_view input, std::size_t& position) {
    if (input.size() - position < 4) {
        throw std::runtime_error("Incomplete unicode escape in LLM response");
    }
    unsigned int value = 0;
    const char* begin = input.data() + position;
    const char* end = begin + 4;
    const auto [parsedEnd, error] = std::from_chars(begin, end, value, 16);
    if (error != std::errc{} || parsedEnd != end) {
        throw std::runtime_error("Invalid unicode escape in LLM response");
    }
    position += 4;
    return value;
}

std::string parseJsonString(std::string_view input, std::size_t position) {
    if (position >= input.size() || input[position] != '"') {
        throw std::runtime_error("Expected content string in LLM response");
    }
    ++position;
    std::string result;
    while (position < input.size()) {
        const char character = input[position++];
        if (character == '"') {
            return result;
        }
        if (character != '\\') {
            result.push_back(character);
            continue;
        }
        if (position >= input.size()) {
            throw std::runtime_error("Incomplete escape in LLM response");
        }
        switch (input[position++]) {
            case '"': result.push_back('"'); break;
            case '\\': result.push_back('\\'); break;
            case '/': result.push_back('/'); break;
            case 'b': result.push_back('\b'); break;
            case 'f': result.push_back('\f'); break;
            case 'n': result.push_back('\n'); break;
            case 'r': result.push_back('\r'); break;
            case 't': result.push_back('\t'); break;
            case 'u': appendUtf8(result, parseHex(input, position)); break;
            default: throw std::runtime_error("Invalid escape in LLM response");
        }
    }
    throw std::runtime_error("Unterminated content string in LLM response");
}

std::string extractContent(std::string_view response) {
    constexpr std::string_view key = "\"content\"";
    std::size_t position = response.find(key);
    while (position != std::string_view::npos) {
        position = response.find(':', position + key.size());
        if (position == std::string_view::npos) {
            break;
        }
        ++position;
        while (
            position < response.size() &&
            (response[position] == ' ' || response[position] == '\r' ||
             response[position] == '\n' || response[position] == '\t')
        ) {
            ++position;
        }
        if (position < response.size() && response[position] == '"') {
            return parseJsonString(response, position);
        }
        position = response.find(key, position);
    }
    throw std::runtime_error("LLM response has no message content");
}

#ifdef _WIN32

std::wstring widen(std::string_view value) {
    if (value.empty()) {
        return {};
    }
    const int size = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        value.data(),
        static_cast<int>(value.size()),
        nullptr,
        0
    );
    if (size <= 0) {
        throw std::runtime_error("Cannot convert API URL to UTF-16");
    }
    std::wstring result(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        value.data(),
        static_cast<int>(value.size()),
        result.data(),
        size
    );
    return result;
}

class InternetHandle {
public:
    explicit InternetHandle(HINTERNET value = nullptr) : value_(value) {}
    InternetHandle(const InternetHandle&) = delete;
    InternetHandle& operator=(const InternetHandle&) = delete;
    ~InternetHandle() {
        if (value_ != nullptr) {
            WinHttpCloseHandle(value_);
        }
    }
    HINTERNET get() const noexcept { return value_; }
    explicit operator bool() const noexcept { return value_ != nullptr; }

private:
    HINTERNET value_;
};

std::runtime_error winHttpError(std::string_view operation) {
    return std::runtime_error(
        std::string(operation) + " failed with WinHTTP error " +
        std::to_string(GetLastError())
    );
}

bool queuedOrPending(bool result) {
    return result || GetLastError() == ERROR_IO_PENDING;
}

void finishWithError(OpenAiAsyncState& state, std::string error) noexcept {
    {
        std::lock_guard lock(state.mutex);
        state.error = std::move(error);
    }
    state.ready.store(true, std::memory_order_release);
}

std::string lastWinHttpError(std::string_view operation, DWORD code) {
    return std::string(operation) + " failed with WinHTTP error " +
           std::to_string(code);
}

void CALLBACK asyncRequestCallback(
    HINTERNET request,
    DWORD_PTR context,
    DWORD internetStatus,
    void* statusInformation,
    DWORD statusInformationLength
) noexcept {
    auto* state = reinterpret_cast<OpenAiAsyncState*>(context);
    if (state == nullptr || state->ready.load(std::memory_order_acquire)) {
        return;
    }

    switch (internetStatus) {
        case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
            if (!queuedOrPending(WinHttpReceiveResponse(request, nullptr))) {
                finishWithError(
                    *state,
                    lastWinHttpError("WinHttpReceiveResponse", GetLastError())
                );
            }
            break;

        case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE: {
            DWORD statusSize = sizeof(state->status);
            if (!WinHttpQueryHeaders(
                    request,
                    WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                    WINHTTP_HEADER_NAME_BY_INDEX,
                    &state->status,
                    &statusSize,
                    WINHTTP_NO_HEADER_INDEX
                )) {
                finishWithError(
                    *state,
                    lastWinHttpError("WinHttpQueryHeaders", GetLastError())
                );
                break;
            }
            if (!queuedOrPending(WinHttpQueryDataAvailable(request, nullptr))) {
                finishWithError(
                    *state,
                    lastWinHttpError(
                        "WinHttpQueryDataAvailable", GetLastError()
                    )
                );
            }
            break;
        }

        case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE: {
            if (statusInformationLength < sizeof(DWORD)) {
                finishWithError(*state, "WinHTTP returned invalid data length");
                break;
            }
            const DWORD available = *static_cast<DWORD*>(statusInformation);
            if (available == 0) {
                state->ready.store(true, std::memory_order_release);
                break;
            }
            const DWORD requested = static_cast<DWORD>(
                (std::min)(
                    static_cast<std::size_t>(available), state->buffer.size()
                )
            );
            if (!queuedOrPending(WinHttpReadData(
                    request, state->buffer.data(), requested, nullptr
                ))) {
                finishWithError(
                    *state,
                    lastWinHttpError("WinHttpReadData", GetLastError())
                );
            }
            break;
        }

        case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
            if (statusInformationLength > 0) {
                std::lock_guard lock(state->mutex);
                state->responseBody.append(
                    static_cast<const char*>(statusInformation),
                    statusInformationLength
                );
            }
            if (!queuedOrPending(WinHttpQueryDataAvailable(request, nullptr))) {
                finishWithError(
                    *state,
                    lastWinHttpError(
                        "WinHttpQueryDataAvailable", GetLastError()
                    )
                );
            }
            break;

        case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR: {
            DWORD code = GetLastError();
            if (statusInformationLength >= sizeof(WINHTTP_ASYNC_RESULT)) {
                code = static_cast<WINHTTP_ASYNC_RESULT*>(statusInformation)
                           ->dwError;
            }
            finishWithError(
                *state, lastWinHttpError("Asynchronous HTTP request", code)
            );
            break;
        }

        default:
            break;
    }
}

void startPostJson(
    const OpenAiClientConfig& config,
    OpenAiAsyncState& state
) {
    std::string endpoint = config.baseUrl;
    while (!endpoint.empty() && endpoint.back() == '/') {
        endpoint.pop_back();
    }
    endpoint += "/chat/completions";
    const std::wstring wideEndpoint = widen(endpoint);

    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);
    parts.dwHostNameLength = static_cast<DWORD>(-1);
    parts.dwUrlPathLength = static_cast<DWORD>(-1);
    parts.dwExtraInfoLength = static_cast<DWORD>(-1);
    if (!WinHttpCrackUrl(wideEndpoint.c_str(), 0, 0, &parts)) {
        throw winHttpError("WinHttpCrackUrl");
    }
    if (parts.nScheme != INTERNET_SCHEME_HTTPS) {
        throw std::invalid_argument("AI base URL must use HTTPS");
    }

    const std::wstring host(parts.lpszHostName, parts.dwHostNameLength);
    std::wstring path(parts.lpszUrlPath, parts.dwUrlPathLength);
    if (parts.dwExtraInfoLength > 0) {
        path.append(parts.lpszExtraInfo, parts.dwExtraInfoLength);
    }

    state.session = WinHttpOpen(
        L"MafiaGame/1.0",
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        WINHTTP_FLAG_ASYNC
    );
    if (state.session == nullptr) throw winHttpError("WinHttpOpen");

    const int timeout = config.timeoutSeconds * 1000;
    if (!WinHttpSetTimeouts(
            state.session, timeout, timeout, timeout, timeout
        )) {
        throw winHttpError("WinHttpSetTimeouts");
    }

    state.connection = WinHttpConnect(
        state.session, host.c_str(), parts.nPort, 0
    );
    if (state.connection == nullptr) throw winHttpError("WinHttpConnect");

    state.request = WinHttpOpenRequest(
        state.connection,
        L"POST",
        path.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE
    );
    if (state.request == nullptr) throw winHttpError("WinHttpOpenRequest");

    const DWORD notifications =
        WINHTTP_CALLBACK_FLAG_SENDREQUEST_COMPLETE |
        WINHTTP_CALLBACK_FLAG_HEADERS_AVAILABLE |
        WINHTTP_CALLBACK_FLAG_DATA_AVAILABLE |
        WINHTTP_CALLBACK_FLAG_READ_COMPLETE |
        WINHTTP_CALLBACK_FLAG_REQUEST_ERROR;
    if (WinHttpSetStatusCallback(
            state.request,
            asyncRequestCallback,
            notifications,
            0
        ) == WINHTTP_INVALID_STATUS_CALLBACK) {
        throw winHttpError("WinHttpSetStatusCallback");
    }

    std::wstring headers = L"Content-Type: application/json\r\n";
    if (!config.apiKey.empty()) {
        headers += L"Authorization: Bearer ";
        headers += widen(config.apiKey);
        headers += L"\r\n";
    }
    if (!queuedOrPending(WinHttpSendRequest(
            state.request,
            headers.c_str(),
            static_cast<DWORD>(headers.size()),
            state.requestBody.data(),
            static_cast<DWORD>(state.requestBody.size()),
            static_cast<DWORD>(state.requestBody.size()),
            reinterpret_cast<DWORD_PTR>(&state)
        ))) {
        throw winHttpError("WinHttpSendRequest");
    }
}

std::string postJson(
    const OpenAiClientConfig& config,
    std::string_view body
) {
    std::string endpoint = config.baseUrl;
    while (!endpoint.empty() && endpoint.back() == '/') {
        endpoint.pop_back();
    }
    endpoint += "/chat/completions";
    const std::wstring wideEndpoint = widen(endpoint);

    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);
    parts.dwHostNameLength = static_cast<DWORD>(-1);
    parts.dwUrlPathLength = static_cast<DWORD>(-1);
    parts.dwExtraInfoLength = static_cast<DWORD>(-1);
    if (!WinHttpCrackUrl(wideEndpoint.c_str(), 0, 0, &parts)) {
        throw winHttpError("WinHttpCrackUrl");
    }
    if (parts.nScheme != INTERNET_SCHEME_HTTPS) {
        throw std::invalid_argument("AI base URL must use HTTPS");
    }

    const std::wstring host(parts.lpszHostName, parts.dwHostNameLength);
    std::wstring path(parts.lpszUrlPath, parts.dwUrlPathLength);
    if (parts.dwExtraInfoLength > 0) {
        path.append(parts.lpszExtraInfo, parts.dwExtraInfoLength);
    }

    InternetHandle session(WinHttpOpen(
        L"MafiaGame/1.0",
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    ));
    if (!session) throw winHttpError("WinHttpOpen");

    const int timeout = config.timeoutSeconds * 1000;
    if (!WinHttpSetTimeouts(session.get(), timeout, timeout, timeout, timeout)) {
        throw winHttpError("WinHttpSetTimeouts");
    }

    InternetHandle connection(WinHttpConnect(
        session.get(), host.c_str(), parts.nPort, 0
    ));
    if (!connection) throw winHttpError("WinHttpConnect");

    InternetHandle request(WinHttpOpenRequest(
        connection.get(),
        L"POST",
        path.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE
    ));
    if (!request) throw winHttpError("WinHttpOpenRequest");

    std::wstring headers = L"Content-Type: application/json\r\n";
    if (!config.apiKey.empty()) {
        headers += L"Authorization: Bearer ";
        headers += widen(config.apiKey);
        headers += L"\r\n";
    }
    if (!WinHttpSendRequest(
            request.get(),
            headers.c_str(),
            static_cast<DWORD>(headers.size()),
            const_cast<char*>(body.data()),
            static_cast<DWORD>(body.size()),
            static_cast<DWORD>(body.size()),
            0
        )) {
        throw winHttpError("WinHttpSendRequest");
    }
    if (!WinHttpReceiveResponse(request.get(), nullptr)) {
        throw winHttpError("WinHttpReceiveResponse");
    }

    DWORD status = 0;
    DWORD statusSize = sizeof(status);
    if (!WinHttpQueryHeaders(
            request.get(),
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &status,
            &statusSize,
            WINHTTP_NO_HEADER_INDEX
        )) {
        throw winHttpError("WinHttpQueryHeaders");
    }

    std::string response;
    while (true) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request.get(), &available)) {
            throw winHttpError("WinHttpQueryDataAvailable");
        }
        if (available == 0) break;
        const std::size_t oldSize = response.size();
        response.resize(oldSize + available);
        DWORD read = 0;
        if (!WinHttpReadData(
                request.get(), response.data() + oldSize, available, &read
            )) {
            throw winHttpError("WinHttpReadData");
        }
        response.resize(oldSize + read);
    }
    if (status < 200 || status >= 300) {
        throw std::runtime_error(
            "LLM API returned HTTP " + std::to_string(status) + ": " +
            response.substr(0, 300)
        );
    }
    return response;
}

#endif

}  // namespace

OpenAiLlmClient::OpenAiLlmClient(OpenAiClientConfig config)
    : config_(std::move(config)) {
    if (config_.baseUrl.empty() || config_.model.empty()) {
        throw std::invalid_argument("AI base URL and model must not be empty");
    }
    if (config_.timeoutSeconds <= 0) {
        throw std::invalid_argument("AI timeout must be positive");
    }
}

OpenAiLlmClient::~OpenAiLlmClient() = default;

std::string OpenAiLlmClient::complete(std::string_view prompt) {
#ifdef _WIN32
    const std::string body =
        "{\"model\":\"" + escapeJson(config_.model) +
        "\",\"messages\":[{\"role\":\"system\",\"content\":"
        "\"Return only valid JSON.\"},{\"role\":\"user\",\"content\":\"" +
        escapeJson(prompt) + "\"}],\"temperature\":0.7}";
    return extractContent(postJson(config_, body));
#else
    static_cast<void>(prompt);
    throw std::runtime_error(
        "OpenAiLlmClient currently requires Windows WinHTTP"
    );
#endif
}

void OpenAiLlmClient::startCompletion(std::string prompt) {
    if (asyncState_ &&
        !asyncState_->ready.load(std::memory_order_acquire)) {
        throw std::logic_error("An LLM completion is already in progress");
    }

    auto state = std::make_unique<OpenAiAsyncState>();
#ifdef _WIN32
    state->requestBody =
        "{\"model\":\"" + escapeJson(config_.model) +
        "\",\"messages\":[{\"role\":\"system\",\"content\":"
        "\"Return only valid JSON.\"},{\"role\":\"user\",\"content\":\"" +
        escapeJson(prompt) + "\"}],\"temperature\":0.7}";
    startPostJson(config_, *state);
#else
    static_cast<void>(prompt);
    state->error = "OpenAiLlmClient currently requires Windows WinHTTP";
    state->ready.store(true, std::memory_order_release);
#endif
    asyncState_ = std::move(state);
}

bool OpenAiLlmClient::completionReady() const noexcept {
    return asyncState_ &&
           asyncState_->ready.load(std::memory_order_acquire);
}

std::string OpenAiLlmClient::takeCompletion() {
    if (!completionReady()) {
        throw std::logic_error("LLM completion is not ready");
    }

    std::string error;
    std::string response;
    unsigned long status = 0;
#ifdef _WIN32
    {
        std::lock_guard lock(asyncState_->mutex);
        error = asyncState_->error;
        response = asyncState_->responseBody;
        status = asyncState_->status;
    }
#else
    error = asyncState_->error;
#endif
    asyncState_.reset();

    if (!error.empty()) {
        throw std::runtime_error(error);
    }
    if (status < 200 || status >= 300) {
        throw std::runtime_error(
            "LLM API returned HTTP " + std::to_string(status) + ": " +
            response.substr(0, 300)
        );
    }
    return extractContent(response);
}

}  // namespace mafia
