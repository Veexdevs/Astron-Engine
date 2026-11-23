#pragma once
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace astron {

struct HttpResponse {
    int status = 0;
    std::string body;
    std::string error;
};

class HttpClient {
public:
    using HeaderList = std::vector<std::pair<std::string, std::string>>;
    using MockHandler = std::function<HttpResponse(const std::string& url, const HeaderList& headers, const std::string& body)>;

    static HttpResponse postJson(
        const std::string& url,
        const HeaderList& headers,
        const std::string& jsonBody,
        int timeoutSec = 90
    );

    static void setMockHandler(MockHandler fn);
};

} // namespace astron
