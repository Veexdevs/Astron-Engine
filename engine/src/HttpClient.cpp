#include "HttpClient.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#elif defined(ASTRON_USE_LIBCURL)
#include <curl/curl.h>
#endif

namespace astron {

namespace {
HttpClient::MockHandler g_mockHandler;

#if defined(_WIN32)
std::wstring toWide(const std::string& utf8) {
    if (utf8.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring w(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), w.data(), n);
    return w;
}

HttpResponse postWinHttp(const std::string& url, const HttpClient::HeaderList& headers, const std::string& body, int timeoutSec) {
    HttpResponse resp;
    std::wstring wurl = toWide(url);

    URL_COMPONENTS uc;
    std::memset(&uc, 0, sizeof(uc));
    uc.dwStructSize = sizeof(uc);
    wchar_t hostBuf[512] = {};
    wchar_t pathBuf[2048] = {};
    uc.lpszHostName = hostBuf;
    uc.dwHostNameLength = 512;
    uc.lpszUrlPath = pathBuf;
    uc.dwUrlPathLength = 2048;

    if (!WinHttpCrackUrl(wurl.c_str(), static_cast<DWORD>(wurl.size()), 0, &uc)) {
        resp.error = "URL inválida para WinHTTP: " + url;
        return resp;
    }

    HINTERNET hSession = WinHttpOpen(L"AstronEngine/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) { resp.error = "Falha em WinHttpOpen"; return resp; }

    int ms = timeoutSec * 1000;
    WinHttpSetTimeouts(hSession, ms, ms, ms, ms);

    HINTERNET hConnect = WinHttpConnect(hSession, hostBuf, uc.nPort, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        resp.error = "Falha ao conectar ao host da API";
        return resp;
    }

    DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hReq = WinHttpOpenRequest(hConnect, L"POST", pathBuf, nullptr,
                                        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hReq) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        resp.error = "Falha ao criar requisição HTTP";
        return resp;
    }

    for (const auto& kv : headers) {
        std::wstring line = toWide(kv.first + ": " + kv.second + "\r\n");
        WinHttpAddRequestHeaders(hReq, line.c_str(), static_cast<DWORD>(line.size()), WINHTTP_ADDREQ_FLAG_ADD);
    }

    BOOL ok = WinHttpSendRequest(
        hReq,
        WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        const_cast<char*>(body.data()),
        static_cast<DWORD>(body.size()),
        static_cast<DWORD>(body.size()),
        0
    );

    if (ok) ok = WinHttpReceiveResponse(hReq, nullptr);
    if (!ok) {
        resp.error = "Falha de rede na chamada à API de IA (WinHTTP)";
        WinHttpCloseHandle(hReq);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return resp;
    }

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    WinHttpQueryHeaders(hReq, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX);
    resp.status = static_cast<int>(statusCode);

    DWORD avail = 0;
    do {
        avail = 0;
        if (!WinHttpQueryDataAvailable(hReq, &avail) || avail == 0) break;
        std::string chunk(avail, '\0');
        DWORD readBytes = 0;
        if (WinHttpReadData(hReq, chunk.data(), avail, &readBytes)) {
            chunk.resize(readBytes);
            resp.body += chunk;
        }
    } while (avail > 0);

    WinHttpCloseHandle(hReq);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return resp;
}
#elif defined(ASTRON_USE_LIBCURL)
size_t curlWriteCb(char* ptr, size_t size, size_t nmemb, void* userdata) {
    size_t total = size * nmemb;
    auto* out = static_cast<std::string*>(userdata);
    out->append(ptr, total);
    return total;
}

HttpResponse postLibCurl(const std::string& url, const HttpClient::HeaderList& headers, const std::string& body, int timeoutSec) {
    HttpResponse resp;
    CURL* curl = curl_easy_init();
    if (!curl) {
        resp.error = "Falha ao inicializar libcurl";
        return resp;
    }

    struct curl_slist* hdrs = nullptr;
    for (const auto& kv : headers) {
        std::string h = kv.first + ": " + kv.second;
        hdrs = curl_slist_append(hdrs, h.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.data());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(timeoutSec));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp.body);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "AstronEngine/1.0");

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        resp.error = std::string("Erro libcurl: ") + curl_easy_strerror(res);
    } else {
        long code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        resp.status = static_cast<int>(code);
    }

    curl_slist_free_all(hdrs);
    curl_easy_cleanup(curl);
    return resp;
}
#else
std::string shellQuoteSingle(const std::string& s) {
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') out += "'\\''";
        else out.push_back(c);
    }
    out.push_back('\'');
    return out;
}

HttpResponse postCurlCli(const std::string& url, const HttpClient::HeaderList& headers, const std::string& body, int timeoutSec) {
    HttpResponse resp;
    namespace fs = std::filesystem;
    auto tmpDir = fs::temp_directory_path();
    std::string tag = std::to_string(std::rand()) + "_" + std::to_string(reinterpret_cast<uintptr_t>(&resp));
    fs::path reqFile  = tmpDir / ("astron_req_" + tag + ".json");
    fs::path respFile = tmpDir / ("astron_resp_" + tag + ".json");

    {
        std::ofstream f(reqFile, std::ios::binary);
        f.write(body.data(), static_cast<std::streamsize>(body.size()));
    }

    std::ostringstream cmd;
    cmd << "curl -sS --max-time " << timeoutSec
        << " -X POST " << shellQuoteSingle(url)
        << " --data-binary @" << shellQuoteSingle(reqFile.string())
        << " -o " << shellQuoteSingle(respFile.string())
        << " -w '%{http_code}'";

    for (const auto& kv : headers) {
        cmd << " -H " << shellQuoteSingle(kv.first + ": " + kv.second);
    }

    FILE* pipe = popen(cmd.str().c_str(), "r");
    if (!pipe) {
        std::error_code ec;
        fs::remove(reqFile, ec);
        fs::remove(respFile, ec);
        resp.error = "Falha ao executar cliente HTTP";
        return resp;
    }

    char codeBuf[64] = {};
    if (std::fgets(codeBuf, sizeof(codeBuf), pipe)) {
        resp.status = std::atoi(codeBuf);
    }
    int rc = pclose(pipe);

    if (fs::exists(respFile)) {
        std::ifstream rf(respFile, std::ios::binary);
        std::ostringstream ss;
        ss << rf.rdbuf();
        resp.body = ss.str();
    }

    std::error_code ec;
    fs::remove(reqFile, ec);
    fs::remove(respFile, ec);

    if (rc != 0 && resp.status == 0) {
        resp.error = "Erro de conexão HTTP com o provedor de IA.";
    }
    return resp;
}
#endif

} // namespace

void HttpClient::setMockHandler(MockHandler fn) {
    g_mockHandler = std::move(fn);
}

HttpResponse HttpClient::postJson(const std::string& url, const HeaderList& headers, const std::string& jsonBody, int timeoutSec) {
    if (g_mockHandler) {
        return g_mockHandler(url, headers, jsonBody);
    }
#if defined(_WIN32)
    return postWinHttp(url, headers, jsonBody, timeoutSec);
#elif defined(ASTRON_USE_LIBCURL)
    return postLibCurl(url, headers, jsonBody, timeoutSec);
#else
    return postCurlCli(url, headers, jsonBody, timeoutSec);
#endif
}

} // namespace astron
