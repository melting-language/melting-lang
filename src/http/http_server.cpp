#include "http_server.hpp"
#include "interpreter.hpp"
#include <charconv>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <cctype>
#include <string_view>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#ifndef TCP_NODELAY
#define TCP_NODELAY 0x0001
#endif
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET (-1)
#define closesocket close
#endif

static ssize_t sendAll(SOCKET sock, const char* data, size_t len) {
    size_t total = 0;
    while (total < len) {
        int sent = send(sock, data + total, (int)(len - total), 0);
        if (sent <= 0) return -1;
        total += (size_t)sent;
    }
    return (ssize_t)total;
}

static std::string getStatusText(int status) {
    switch (status) {
        case 200: return "OK";
        case 302: return "Found";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        default: return "Error";
    }
}

static size_t findHeaderValue(const char* data, size_t len, const char* name) {
    size_t nameLen = std::strlen(name);
    size_t pos = 0;
    while (pos < len) {
        size_t lineEnd = pos;
        while (lineEnd < len && data[lineEnd] != '\n') ++lineEnd;
        size_t start = pos;
        while (start < lineEnd && (data[start] == ' ' || data[start] == '\t')) ++start;
        if (start + nameLen < lineEnd) {
            bool match = true;
            for (size_t i = 0; i < nameLen; ++i) {
                char a = data[start + i];
                char b = name[i];
                if (a >= 'A' && a <= 'Z') a += 'a' - 'A';
                if (a != b) {
                    match = false;
                    break;
                }
            }
            if (match && start + nameLen < lineEnd && data[start + nameLen] == ':') {
                size_t valuePos = start + nameLen + 1;
                while (valuePos < lineEnd && (data[valuePos] == ' ' || data[valuePos] == '\t')) ++valuePos;
                return valuePos;
            }
        }
        pos = (lineEnd < len) ? lineEnd + 1 : lineEnd;
    }
    return std::string::npos;
}

static bool parseContentLength(const char* data, size_t len, size_t& contentLen) {
    size_t pos = findHeaderValue(data, len, "content-length");
    if (pos == std::string::npos) return false;
    const char* start = data + pos;
    const char* end = data + len;
    while (start < end && (*start == ' ' || *start == '\t')) ++start;
    if (start >= end || *start < '0' || *start > '9') return false;
    const char* numEnd = start;
    while (numEnd < end && *numEnd >= '0' && *numEnd <= '9') ++numEnd;
    auto result = std::from_chars(start, numEnd, contentLen);
    return result.ec == std::errc() && numEnd > start;
}

static void parseRequest(const std::string& raw, std::string& method, std::string& path, std::string& body, std::string& headersOut) {
    method = "GET";
    path = "/";
    body.clear();
    headersOut.clear();
    size_t line1 = raw.find("\r\n");
    if (line1 == std::string::npos) line1 = raw.find('\n');
    if (line1 == std::string::npos) return;
    std::string_view first(raw.data(), line1);
    size_t s1 = first.find(' ');
    if (s1 == std::string_view::npos) return;
    size_t s2 = first.find(' ', s1 + 1);
    if (s2 != std::string_view::npos) {
        method.assign(first.data(), s1);
        path.assign(first.data() + s1 + 1, s2 - (s1 + 1));
    }
    size_t headEnd = raw.find("\r\n\r\n");
    bool useCrlf = (headEnd != std::string::npos);
    if (headEnd == std::string::npos) headEnd = raw.find("\n\n");
    if (headEnd == std::string::npos) return;
    size_t headersStart = line1 + (useCrlf ? 2u : 1u);
    headersOut.assign(raw, headersStart, headEnd - headersStart);
    size_t bodyStart = headEnd + (useCrlf ? 4u : 2u);
    if (bodyStart > raw.size()) return;
    body.assign(raw, bodyStart, raw.size() - bodyStart);
    size_t contentLen = 0;
    if (parseContentLength(raw.data(), headEnd, contentLen) && contentLen <= body.size())
        body.resize(contentLen);
}

static void handleOneRequest(Interpreter* interp, SOCKET clientFd, const std::string& request) {
    std::string method, path, body, headers;
    parseRequest(request, method, path, body, headers);
    interp->setRequestData(path, method, body, headers);
    bool firstChunk = true;
    interp->setResponseChunkSender([clientFd, &firstChunk](Interpreter* i, const std::string& chunk) {
        if (firstChunk) {
            int status = i->getResponseStatus();
            std::string statusText = getStatusText(status);
            std::string hdr;
            hdr.reserve(256);
            hdr += "HTTP/1.1 ";
            hdr += std::to_string(status);
            hdr += " ";
            hdr += statusText;
            hdr += "\r\nTransfer-Encoding: chunked\r\nConnection: close\r\nContent-Type: ";
            hdr += i->getResponseContentType();
            hdr += "\r\n";
            for (const auto& h : i->getResponseHeaders()) {
                hdr += h.first;
                hdr += ": ";
                hdr += h.second;
                hdr += "\r\n";
            }
            hdr += "\r\n";
            sendAll(clientFd, hdr.data(), hdr.size());
            firstChunk = false;
        }
        if (chunk.empty()) return;
        char lenBuf[32];
        int n = snprintf(lenBuf, sizeof(lenBuf), "%zx\r\n", chunk.size());
        sendAll(clientFd, lenBuf, (size_t)n);
        sendAll(clientFd, chunk.data(), chunk.size());
        sendAll(clientFd, "\r\n", 2);
    });
    try {
        interp->callHandler();
    } catch (const std::exception& e) {
        interp->setResponseChunkSender(nullptr);
        std::string raw(e.what());
        std::string msg;
        msg.reserve(raw.size() + 32);
        for (char c : raw) {
            if (c == '&') msg += "&amp;";
            else if (c == '<') msg += "&lt;";
            else if (c == '>') msg += "&gt;";
            else if (c == '"') msg += "&quot;";
            else msg += c;
        }
        std::ostringstream html;
        html << "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Error</title>"
            << "<style>body{font-family:sans-serif;max-width:600px;margin:2rem auto;padding:0 1rem;} "
            << ".err{background:#fef2f2;border:1px solid #fecaca;color:#b91c1c;padding:1rem;border-radius:0.5rem;white-space:pre-wrap;word-break:break-all;} "
            << "h1{color:#991b1b;}</style></head><body>"
            << "<h1>Server error</h1><p class=\"err\">" << msg << "</p>"
            << "<p><a href=\"/\">Back to app</a></p></body></html>";
        std::string bodyStr = html.str();
        std::string response;
        response.reserve(128 + bodyStr.size());
        response += "HTTP/1.1 500 Internal Server Error\r\n";
        response += "Content-Length: ";
        response += std::to_string(bodyStr.size());
        response += "\r\nConnection: close\r\nContent-Type: text/html; charset=utf-8\r\n\r\n";
        response += bodyStr;
        sendAll(clientFd, response.data(), response.size());
        closesocket(clientFd);
        return;
    }
    interp->setResponseChunkSender(nullptr);
    if (interp->responseStreamingUsed()) {
        const char term[] = "0\r\n\r\n";
        sendAll(clientFd, term, (size_t)sizeof(term) - 1);
    } else {
        std::string respBody = interp->getResponseBody();
        int status = interp->getResponseStatus();
        std::string contentType = interp->getResponseContentType();
        std::string statusText = getStatusText(status);
        std::string response;
        response.reserve(128 + respBody.size());
        response += "HTTP/1.1 ";
        response += std::to_string(status);
        response += " ";
        response += statusText;
        response += "\r\nContent-Length: ";
        response += std::to_string(respBody.size());
        response += "\r\nConnection: close\r\nContent-Type: ";
        response += contentType;
        response += "\r\n";
        for (const auto& h : interp->getResponseHeaders()) {
            response += h.first;
            response += ": ";
            response += h.second;
            response += "\r\n";
        }
        response += "\r\n";
        response += respBody;
        sendAll(clientFd, response.data(), response.size());
    }
    closesocket(clientFd);
}

void runHttpServer(Interpreter* interp, int port) {
#if defined(_WIN32) || defined(_WIN64)
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        throw std::runtime_error("WSAStartup failed");
#endif
    SOCKET listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd == INVALID_SOCKET)
        throw std::runtime_error("socket failed");
    int opt = 1;
#if defined(_WIN32) || defined(_WIN64)
    setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((unsigned short)port);
    if (bind(listenFd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        closesocket(listenFd);
        throw std::runtime_error("bind failed on port " + std::to_string(port));
    }
    if (listen(listenFd, 5) != 0) {
        closesocket(listenFd);
        throw std::runtime_error("listen failed");
    }
#if defined(_WIN32) || defined(_WIN64)
    std::queue<std::pair<SOCKET, std::string>> requestQueue;
    std::mutex queueMutex;
    std::condition_variable queueCond;
    std::thread worker([interp, &requestQueue, &queueMutex, &queueCond]() {
        for (;;) {
            std::pair<SOCKET, std::string> item;
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                queueCond.wait(lock, [&]() { return !requestQueue.empty(); });
                item = std::move(requestQueue.front());
                requestQueue.pop();
            }
            handleOneRequest(interp, item.first, item.second);
        }
    });
#endif
    for (;;) {
        struct sockaddr_in clientAddr;
#if defined(_WIN32) || defined(_WIN64)
        int clientLen = sizeof(clientAddr);
#else
        socklen_t clientLen = sizeof(clientAddr);
#endif
        SOCKET clientFd = accept(listenFd, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientFd == INVALID_SOCKET) continue;
        int one = 1;
        setsockopt(clientFd, IPPROTO_TCP, TCP_NODELAY, (const char*)&one, sizeof(one));
        std::string request;
        char buf[4096];
        size_t expectedBodyLen = 0;
        bool haveHeaders = false;
        size_t bodyStart = std::string::npos;
        for (;;) {
            int n = recv(clientFd, buf, sizeof(buf), 0);
            if (n <= 0) break;
            request.append(buf, (size_t)n);

            if (!haveHeaders) {
                size_t headEnd = request.find("\r\n\r\n");
                size_t sepLen = 4;
                if (headEnd == std::string::npos) {
                    headEnd = request.find("\n\n");
                    sepLen = 2;
                }
                if (headEnd != std::string::npos) {
                    haveHeaders = true;
                    bodyStart = headEnd + sepLen;
                    size_t contentLen = 0;
                    if (parseContentLength(request.data(), headEnd, contentLen)) {
                        expectedBodyLen = contentLen;
                    }
                    if (request.size() >= bodyStart + expectedBodyLen) break;
                }
            } else if (bodyStart != std::string::npos) {
                if (request.size() >= bodyStart + expectedBodyLen) break;
            }
        }
#if defined(_WIN32) || defined(_WIN64)
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            requestQueue.push({clientFd, std::move(request)});
        }
        queueCond.notify_one();
#else
        (void)interp;
        while (waitpid(-1, NULL, WNOHANG) > 0) {}
        pid_t pid = fork();
        if (pid == 0) {
            closesocket(listenFd);
            handleOneRequest(interp, clientFd, request);
            _exit(0);
        }
        closesocket(clientFd);
#endif
    }
    closesocket(listenFd);
#if defined(_WIN32) || defined(_WIN64)
    WSACleanup();
#endif
}
