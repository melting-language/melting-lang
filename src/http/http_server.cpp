#include "http_server.hpp"
#include "interpreter.hpp"
#include <cstring>
#include <sstream>
#include <stdexcept>

#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
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
            size_t cl = headers.find("Content-Length:");
            if (cl != std::string::npos) {
                cl = headers.find_first_not_of(" \t", cl + 14);
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
    for (;;) {
        struct sockaddr_in clientAddr;
#if defined(_WIN32) || defined(_WIN64)
        int clientLen = sizeof(clientAddr);
#else
        socklen_t clientLen = sizeof(clientAddr);
#endif
        SOCKET clientFd = accept(listenFd, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientFd == INVALID_SOCKET) continue;
        std::string request;
        char buf[4096];
        for (;;) {
            int n = recv(clientFd, buf, sizeof(buf) - 1, 0);
            if (n <= 0) break;
            buf[n] = '\0';
            request += buf;
            if (request.find("\r\n\r\n") != std::string::npos || request.find("\n\n") != std::string::npos)
                break;
        }
        std::string method, path, body, headers;
        parseRequest(request, method, path, body, headers);
        interp->setRequestData(path, method, body, headers);
        interp->callHandler();
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
        closesocket(clientFd);
    }
    closesocket(listenFd);
#if defined(_WIN32) || defined(_WIN64)
    WSACleanup();
#endif
}
