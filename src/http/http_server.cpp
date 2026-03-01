#include "http_server.hpp"
#include "interpreter.hpp"
#include <cstdio>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <cctype>
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

static void parseRequest(const std::string& raw, std::string& method, std::string& path, std::string& body, std::string& headersOut) {
    method = "GET";
    path = "/";
    body = "";
    headersOut = "";
    size_t line1 = raw.find("\r\n");
    if (line1 == std::string::npos) line1 = raw.find('\n');
    if (line1 == std::string::npos) return;
    std::string first = raw.substr(0, line1);
    size_t s1 = first.find(' ');
    if (s1 == std::string::npos) return;
    size_t s2 = first.find(' ', s1 + 1);
    if (s2 != std::string::npos) {
        method = first.substr(0, s1);
        path = first.substr(s1 + 1, s2 - (s1 + 1));
    }
    size_t headEnd = raw.find("\r\n\r\n");
    bool useCrlf = (headEnd != std::string::npos);
    if (headEnd == std::string::npos) headEnd = raw.find("\n\n");
    if (headEnd != std::string::npos) {
        size_t headersStart = line1 + (raw.substr(line1, 2) == "\r\n" ? 2u : 1u);
        headersOut = raw.substr(headersStart, headEnd - headersStart);
        size_t bodyStart = headEnd + (useCrlf ? 4u : 2u);
        if (bodyStart <= raw.size()) {
            body = raw.substr(bodyStart);
            std::string headers = raw.substr(0, headEnd);
            std::string headersLower = headers;
            for (char& ch : headersLower) ch = (char)std::tolower((unsigned char)ch);
            size_t cl = headersLower.find("content-length:");
            if (cl != std::string::npos) {
                cl = headers.find_first_not_of(" \t", cl + 15);
                if (cl != std::string::npos) {
                    size_t end = cl;
                    while (end < headers.size() && (unsigned char)headers[end] >= '0' && (unsigned char)headers[end] <= '9') ++end;
                    if (end > cl) {
                        int len = std::stoi(headers.substr(cl, end - cl));
                        if (len >= 0 && (size_t)len <= body.size()) body = body.substr(0, (size_t)len);
                    }
                }
            }
        }
    }
}

static void handleOneRequest(Interpreter* interp, SOCKET clientFd, const std::string& request) {
    std::string method, path, body, headers;
    parseRequest(request, method, path, body, headers);
    interp->setRequestData(path, method, body, headers);
    bool firstChunk = true;
    interp->setResponseChunkSender([clientFd, &firstChunk](Interpreter* i, const std::string& chunk) {
        if (firstChunk) {
            int status = i->getResponseStatus();
            std::string statusText = (status == 200) ? "OK" : (status == 404) ? "Not Found" : (status == 302) ? "Found" : "Error";
            std::ostringstream head;
            head << "HTTP/1.1 " << status << " " << statusText << "\r\n"
                << "Transfer-Encoding: chunked\r\n"
                << "Connection: close\r\n"
                << "Content-Type: " << i->getResponseContentType() << "\r\n";
            for (const auto& h : i->getResponseHeaders())
                head << h.first << ": " << h.second << "\r\n";
            head << "\r\n";
            std::string hdr = head.str();
            send(clientFd, hdr.data(), (int)hdr.size(), 0);
            firstChunk = false;
        }
        if (chunk.empty()) return;
        char lenBuf[32];
        int n = snprintf(lenBuf, sizeof(lenBuf), "%zx\r\n", chunk.size());
        send(clientFd, lenBuf, n, 0);
        send(clientFd, chunk.data(), (int)chunk.size(), 0);
        send(clientFd, "\r\n", 2, 0);
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
        std::ostringstream out;
        out << "HTTP/1.1 500 Internal Server Error\r\n"
            << "Content-Length: " << bodyStr.size() << "\r\n"
            << "Connection: close\r\n"
            << "Content-Type: text/html; charset=utf-8\r\n\r\n" << bodyStr;
        std::string response = out.str();
        send(clientFd, response.data(), (int)response.size(), 0);
        closesocket(clientFd);
        return;
    }
    interp->setResponseChunkSender(nullptr);
    if (interp->responseStreamingUsed()) {
        const char term[] = "0\r\n\r\n";
        send(clientFd, term, (int)sizeof(term) - 1, 0);
    } else {
        std::string respBody = interp->getResponseBody();
        int status = interp->getResponseStatus();
        std::string contentType = interp->getResponseContentType();
        std::string statusText = (status == 200) ? "OK" : (status == 404) ? "Not Found" : (status == 302) ? "Found" : "Error";
        std::ostringstream out;
        out << "HTTP/1.1 " << status << " " << statusText << "\r\n"
            << "Content-Length: " << respBody.size() << "\r\n"
            << "Connection: close\r\n"
            << "Content-Type: " << contentType << "\r\n";
        for (const auto& h : interp->getResponseHeaders())
            out << h.first << ": " << h.second << "\r\n";
        out << "\r\n" << respBody;
        std::string response = out.str();
        send(clientFd, response.data(), (int)response.size(), 0);
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
                    std::string head = request.substr(0, headEnd);
                    std::string headLower = head;
                    for (char& ch : headLower) ch = (char)std::tolower((unsigned char)ch);
                    size_t cl = headLower.find("content-length:");
                    if (cl != std::string::npos) {
                        cl = headLower.find_first_not_of(" \t", cl + 15);
                        if (cl != std::string::npos) {
                            size_t end = cl;
                            while (end < headLower.size() && headLower[end] >= '0' && headLower[end] <= '9') ++end;
                            if (end > cl) expectedBodyLen = (size_t)std::stoul(headLower.substr(cl, end - cl));
                        }
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
