#include "interpreter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "http_server.hpp"
#include "mysql_builtin.hpp"
#include "gui_window.hpp"
#include "module_loader.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <random>

struct ReturnException : std::exception {
    Value value;
    explicit ReturnException(Value v) : value(std::move(v)) {}
    const char* what() const noexcept override { return "return"; }
};

struct ThrowException : std::exception {
    Value value;
    explicit ThrowException(Value v) : value(std::move(v)) {}
    const char* what() const noexcept override { return "throw"; }
};

static const char kBase64Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string base64Encode(const std::string& in) {
    std::string out;
    int val = 0, valb = -6;
    for (unsigned char c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(kBase64Chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(kBase64Chars[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

static std::string base64Decode(const std::string& in) {
    std::string out;
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T[(unsigned char)kBase64Chars[i]] = i;
    int val = 0, valb = -8;
    for (unsigned char c : in) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

static std::string xorCipher(const std::string& data, const std::string& key) {
    if (key.empty()) return data;
    std::string out;
    out.reserve(data.size());
    for (size_t i = 0; i < data.size(); i++)
        out.push_back(static_cast<char>(static_cast<unsigned char>(data[i]) ^ static_cast<unsigned char>(key[i % key.size()])));
    return out;
}

static std::string htmlEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s) {
        if (c == '&') out += "&amp;";
        else if (c == '<') out += "&lt;";
        else if (c == '>') out += "&gt;";
        else if (c == '"') out += "&quot;";
        else if (c == '\'') out += "&#39;";
        else out += c;
    }
    return out;
}

static std::string valueToViewString(const Value& v) {
    if (std::holds_alternative<std::monostate>(v)) return "";
    if (std::holds_alternative<std::string>(v)) return std::get<std::string>(v);
    if (std::holds_alternative<double>(v)) {
        double d = std::get<double>(v);
        if (d == static_cast<long long>(d))
            return std::to_string(static_cast<long long>(d));
        return std::to_string(d);
    }
    if (std::holds_alternative<bool>(v)) return std::get<bool>(v) ? "true" : "false";
    if (std::holds_alternative<std::shared_ptr<MeltObject>>(v)) return "[object]";
    if (std::holds_alternative<std::shared_ptr<MeltArray>>(v)) return "[array]";
    if (std::holds_alternative<std::shared_ptr<MeltVec>>(v)) return "[vector]";
    return "";
}

static std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (unsigned char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if (c < 32) { char b[8]; snprintf(b, sizeof(b), "\\u%04x", c); out += b; }
        else out += c;
    }
    return out;
}

static std::string valueToJson(const Value& v) {
    if (std::holds_alternative<std::monostate>(v)) return "null";
    if (std::holds_alternative<double>(v)) {
        double d = std::get<double>(v);
        if (d == static_cast<long long>(d))
            return std::to_string(static_cast<long long>(d));
        return std::to_string(d);
    }
    if (std::holds_alternative<std::string>(v))
        return "\"" + jsonEscape(std::get<std::string>(v)) + "\"";
    if (std::holds_alternative<bool>(v))
        return std::get<bool>(v) ? "true" : "false";
    if (std::holds_alternative<std::shared_ptr<MeltArray>>(v)) {
        auto& a = *std::get<std::shared_ptr<MeltArray>>(v);
        std::string out = "[";
        for (size_t i = 0; i < a.data.size(); ++i) {
            if (i) out += ",";
            out += valueToJson(a.data[i]);
        }
        return out + "]";
    }
    if (std::holds_alternative<std::shared_ptr<MeltObject>>(v)) {
        auto& o = *std::get<std::shared_ptr<MeltObject>>(v);
        std::string out = "{";
        bool first = true;
        for (auto& kv : o.fields) {
            if (!first) out += ",";
            first = false;
            out += "\"" + jsonEscape(kv.first) + "\":" + valueToJson(kv.second);
        }
        return out + "}";
    }
    if (std::holds_alternative<std::shared_ptr<MeltVec>>(v)) {
        auto& vec = *std::get<std::shared_ptr<MeltVec>>(v);
        if (vec.dim == 2)
            return "[" + std::to_string(vec.x) + "," + std::to_string(vec.y) + "]";
        return "[" + std::to_string(vec.x) + "," + std::to_string(vec.y) + "," + std::to_string(vec.z) + "]";
    }
    return "null";
}

static const char* skipWs(const char* p, const char* end) {
    while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) ++p;
    return p;
}

static Value parseJsonValue(Interpreter* interp, const char*& p, const char* end);

static Value parseJsonString(const char*& p, const char* end) {
    if (p >= end || *p != '"') return std::string("");
    ++p;
    std::string s;
    while (p < end && *p != '"') {
        if (*p == '\\') {
            if (++p >= end) return s;
            if (*p == 'n') s += '\n';
            else if (*p == 'r') s += '\r';
            else if (*p == 't') s += '\t';
            else if (*p == '"' || *p == '\\') s += *p;
            else s += *p;
            ++p;
        } else
            s += *p++;
    }
    if (p < end) ++p;
    return s;
}

static Value parseJsonValue(Interpreter* interp, const char*& p, const char* end) {
    p = skipWs(p, end);
    if (p >= end) return false;
    if (*p == '"') return parseJsonString(p, end);
    if (*p == '[') {
        ++p;
        auto arr = std::make_shared<MeltArray>();
        p = skipWs(p, end);
        if (p < end && *p != ']') {
            for (;;) {
                arr->data.push_back(parseJsonValue(interp, p, end));
                p = skipWs(p, end);
                if (p >= end || *p != ',') break;
                ++p;
            }
        }
        if (p < end && *p == ']') ++p;
        return arr;
    }
    if (*p == '{') {
        ++p;
        auto obj = std::make_shared<MeltObject>();
        obj->klass = interp->getJsonObjectClass();
        p = skipWs(p, end);
        if (p < end && *p != '}') {
            for (;;) {
                if (p >= end || *p != '"') break;
                Value keyVal = parseJsonString(p, end);
                if (!std::holds_alternative<std::string>(keyVal)) break;
                std::string key = std::get<std::string>(keyVal);
                p = skipWs(p, end);
                if (p >= end || *p != ':') break;
                ++p;
                obj->fields[key] = parseJsonValue(interp, p, end);
                p = skipWs(p, end);
                if (p >= end || *p != ',') break;
                ++p;
            }
        }
        if (p < end && *p == '}') ++p;
        return obj;
    }
    if (p + 4 <= end && (p[0]=='t'||p[0]=='T') && (p[1]=='r'||p[1]=='R') && (p[2]=='u'||p[2]=='U') && (p[3]=='e'||p[3]=='E')) {
        p += 4; return true;
    }
    if (p + 5 <= end && (p[0]=='f'||p[0]=='F') && (p[1]=='a'||p[1]=='A') && (p[2]=='l'||p[2]=='L') && (p[3]=='s'||p[3]=='S') && (p[4]=='e'||p[4]=='E')) {
        p += 5; return false;
    }
    if (p + 4 <= end && (p[0]=='n'||p[0]=='N') && (p[1]=='u'||p[1]=='U') && (p[2]=='l'||p[2]=='L') && (p[3]=='l'||p[3]=='L')) {
        p += 4; return false;
    }
    if (*p == '-' || (*p >= '0' && *p <= '9')) {
        const char* start = p;
        if (*p == '-') ++p;
        while (p < end && *p >= '0' && *p <= '9') ++p;
        if (p < end && *p == '.') {
            ++p;
            while (p < end && *p >= '0' && *p <= '9') ++p;
        }
        if (p > start && (p - start != 1 || *start != '-'))
            return std::stod(std::string(start, p));
    }
    return false;
}

static std::string dirname(const std::string& path) {
    if (path.empty()) return "";
    size_t i = path.size();
    while (i > 0 && path[i - 1] != '/' && path[i - 1] != '\\') --i;
    if (i == 0) return "";
    return path.substr(0, i - 1);
}

static bool fileExists(const std::string& path) {
    std::ifstream f(path);
    return f.good();
}

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end == std::string::npos ? std::string::npos : end - start + 1);
}

void Interpreter::setBinDir(const std::string& binDir) {
    binDir_ = binDir;
}

void Interpreter::loadGlobalConfig() {
    if (binDir_.empty()) return;
    std::string configPath = binDir_ + "/melt.ini";
    std::ifstream f(configPath);
    if (!f) return;
    config_.clear();
    modulePath_.clear();
    std::string line;
    while (std::getline(f, line)) {
        size_t comment = line.find('#');
        if (comment != std::string::npos) line = line.substr(0, comment);
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = trim(line.substr(0, eq));
        std::string value = trim(line.substr(eq + 1));
        if (key.empty()) continue;
        config_[key] = value;
        if (key == "modulePath") {
            size_t pos = 0;
            while (pos < value.size()) {
                size_t next = value.find(',', pos);
                std::string part = trim(next == std::string::npos ? value.substr(pos) : value.substr(pos, next - pos));
                if (!part.empty()) modulePath_.push_back(binDir_ + "/" + part);
                if (next == std::string::npos) break;
                pos = next + 1;
            }
        }
    }
}

void Interpreter::loadConfig(const std::string& entryDir) {
    if (entryDir.empty()) return;
    std::string configPath = entryDir + "/melt.config";
    std::ifstream f(configPath);
    if (!f) return;
    std::string line;
    while (std::getline(f, line)) {
        size_t comment = line.find('#');
        if (comment != std::string::npos) line = line.substr(0, comment);
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = trim(line.substr(0, eq));
        std::string value = trim(line.substr(eq + 1));
        if (key.empty()) continue;
        config_[key] = value;
        if (key == "modulePath") {
            size_t pos = 0;
            while (pos < value.size()) {
                size_t next = value.find(',', pos);
                std::string part = trim(next == std::string::npos ? value.substr(pos) : value.substr(pos, next - pos));
                if (!part.empty()) modulePath_.push_back(entryDir + "/" + part);
                if (next == std::string::npos) break;
                pos = next + 1;
            }
        }
    }
}

std::string Interpreter::resolveImportPath(const std::string& path) {
    std::string withExt = path;
    if (path.find('.') == std::string::npos)
        withExt = path + ".melt";
    auto tryPath = [](const std::string& base, const std::string& p) -> std::string {
        if (base.empty()) return p;
        return base + "/" + p;
    };
    std::string candidate = tryPath(currentDir_, withExt);
    if (fileExists(candidate)) return candidate;
    candidate = tryPath(currentDir_, path);
    if (fileExists(candidate)) return candidate;
    for (const std::string& base : modulePath_) {
        candidate = tryPath(base, withExt);
        if (fileExists(candidate)) return candidate;
        candidate = tryPath(base, path);
        if (fileExists(candidate)) return candidate;
    }
    throw std::runtime_error("Module not found: " + path);
}

std::string Interpreter::resolvePath(const std::string& path) {
    if (currentDir_.empty()) return path;
    return currentDir_ + "/" + path;
}

std::string Interpreter::getResolvedPath(const std::string& path) const {
    if (currentDir_.empty() || path.empty()) return path;
    if (path[0] == '/' || (path.size() >= 2 && path[1] == ':')) return path;
    return currentDir_ + "/" + path;
}

void Interpreter::addModulePath(const std::string& dir) {
    if (!dir.empty()) modulePath_.push_back(dir);
}

std::string Interpreter::getConfig(const std::string& key) const {
    auto it = config_.find(key);
    return it != config_.end() ? it->second : "";
}

static std::string contentTypeFromPath(const std::string& path) {
    size_t dot = path.rfind('.');
    if (dot == std::string::npos) return "application/octet-stream";
    std::string ext;
    for (size_t i = dot + 1; i < path.size(); ++i)
        ext += (char)std::tolower((unsigned char)path[i]);
    if (ext == "js") return "application/javascript";
    if (ext == "css") return "text/css; charset=utf-8";
    if (ext == "png") return "image/png";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "gif") return "image/gif";
    if (ext == "ico") return "image/x-icon";
    if (ext == "svg") return "image/svg+xml";
    return "application/octet-stream";
}

bool Interpreter::servePublic(const std::string& path) {
    if (path.empty() || path[0] != '/') return false;
    if (path.find("/js/") != 0 && path.find("/css/") != 0 && path.find("/images/") != 0)
        return false;
    std::string filePath = currentDir_.empty() ? ("public" + path) : (currentDir_ + "/public" + path);
    std::ifstream f(filePath, std::ios::binary);
    if (!f) return false;
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    f.close();
    responseBody_ = std::move(content);
    responseContentType_ = contentTypeFromPath(path);
    responseStatus_ = 200;
    return true;
}

std::string Interpreter::readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Cannot open file: " + path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::string Interpreter::renderViewTemplate(const std::string& path, std::shared_ptr<MeltObject> obj) {
    std::string fullPath = resolvePath(path);
    std::string content = readFile(fullPath);
    if (!obj) return content;
    for (const auto& kv : obj->fields) {
        const std::string& key = kv.first;
        std::string val = valueToViewString(kv.second);
        std::string escaped = htmlEscape(val);
        std::string place = "{{ " + key + " }}";
        std::string rawPlace = "{!! " + key + " !!}";
        for (size_t pos = 0; (pos = content.find(place, pos)) != std::string::npos; pos += escaped.size())
            content.replace(pos, place.size(), escaped);
        for (size_t pos = 0; (pos = content.find(rawPlace, pos)) != std::string::npos; pos += val.size())
            content.replace(pos, rawPlace.size(), val);
    }
    return content;
}

static void imageDrawLineBresenham(std::vector<uint8_t>& data, int w, int h, int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b) {
    if (w <= 0 || h <= 0 || data.size() != (size_t)(w * h * 3)) return;
    int dx = std::abs(x2 - x1), sx = (x1 < x2) ? 1 : -1;
    int dy = -std::abs(y2 - y1), sy = (y1 < y2) ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        if (x1 >= 0 && x1 < w && y1 >= 0 && y1 < h) {
            size_t i = (y1 * w + x1) * 3;
            data[i] = r; data[i + 1] = g; data[i + 2] = b;
        }
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

bool Interpreter::saveImagePpm(const std::string& path) const {
    if (imageWidth_ <= 0 || imageHeight_ <= 0 || imageData_.size() != (size_t)(imageWidth_ * imageHeight_ * 3))
        return false;
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f << "P6\n" << imageWidth_ << " " << imageHeight_ << "\n255\n";
    f.write(reinterpret_cast<const char*>(imageData_.data()), imageData_.size());
    return !!f;
}

void Interpreter::interpret(const std::vector<std::unique_ptr<Stmt>>& statements,
                            const std::string& currentFilePath) {
    currentDir_ = dirname(currentFilePath);
    loadGlobalConfig();
    loadConfig(currentDir_);
    if (nativeFunctions_.empty()) registerBuiltins();
    std::string extEnabled = getConfig("extension_enabled");
    bool loadExt = true;
    if (!extEnabled.empty()) {
        std::string v = extEnabled;
        for (char& c : v) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (v == "0" || v == "false" || v == "off" || v == "no") loadExt = false;
    }
    if (loadExt) {
        std::string extDir = getConfig("extension_dir");
        if (extDir.empty()) extDir = "modules";
        std::string extList = getConfig("extension");
        if (!extList.empty() && !binDir_.empty())
            loadExtensions(this, binDir_, extDir, extList);
    }
    for (const auto& stmt : statements) {
        execute(*stmt);
    }
}

void Interpreter::setRequestData(const std::string& path, const std::string& method, const std::string& body, const std::string& headers) {
    currentRequestPath_ = path;
    currentRequestMethod_ = method;
    currentRequestBody_ = body;
    currentRequestHeaders_ = headers;
    responseBody_ = "";
    responseStatus_ = 200;
    responseContentType_ = "text/html; charset=utf-8";
    responseHeaders_.clear();
}

void Interpreter::callHandler() {
    auto it = variables_.find(handlerClassName_);
    if (it == variables_.end() || !std::holds_alternative<std::shared_ptr<MeltClass>>(it->second))
        throw std::runtime_error("Handler not set or invalid: " + handlerClassName_);
    auto klass = std::get<std::shared_ptr<MeltClass>>(it->second);
    auto mit = klass->methods.find("handle");
    if (mit == klass->methods.end())
        throw std::runtime_error("Handler class has no handle() method: " + handlerClassName_);
    auto obj = std::make_shared<MeltObject>();
    obj->klass = klass;
    Value prevThis = this_;
    this_ = obj;
    const MeltMethod& m = mit->second;
    for (const auto& s : m.body->statements) execute(*s);
    this_ = prevThis;
}

void Interpreter::setMcpRequest(const std::string& s) { mcpRequest_ = s; }
std::string Interpreter::getMcpRequest() const { return mcpRequest_; }
void Interpreter::setMcpResponse(const std::string& s) { mcpResponse_ = s; }
std::string Interpreter::getMcpResponse() const { return mcpResponse_; }
void Interpreter::setMcpHandlerClassName(const std::string& s) { mcpHandlerClassName_ = s; }

void Interpreter::callMcpHandler() {
    auto it = variables_.find(mcpHandlerClassName_);
    if (it == variables_.end() || !std::holds_alternative<std::shared_ptr<MeltClass>>(it->second))
        throw std::runtime_error("MCP handler not set or invalid: " + mcpHandlerClassName_);
    auto klass = std::get<std::shared_ptr<MeltClass>>(it->second);
    auto mit = klass->methods.find("handle");
    if (mit == klass->methods.end())
        throw std::runtime_error("MCP handler class has no handle() method: " + mcpHandlerClassName_);
    auto obj = std::make_shared<MeltObject>();
    obj->klass = klass;
    Value prevThis = this_;
    this_ = obj;
    const MeltMethod& m = mit->second;
    for (const auto& s : m.body->statements) execute(*s);
    this_ = prevThis;
}

void Interpreter::registerBuiltins() {
    auto reg = [this](NativeFn f) {
        size_t i = nativeFunctions_.size();
        nativeFunctions_.push_back(std::move(f));
        return NativeFunc{i};
    };
    variables_["getRequestPath"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)args;
        return i->getCurrentRequestPath();
    });
    variables_["getRequestMethod"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)args;
        return i->getCurrentRequestMethod();
    });
    variables_["getRequestBody"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)args;
        return i->getCurrentRequestBody();
    });
    variables_["setResponseBody"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        if (!args.empty() && std::holds_alternative<std::string>(args[0]))
            i->setResponseBodyInternal(std::get<std::string>(args[0]));
        return false;
    });
    variables_["setResponseStatus"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        if (!args.empty() && std::holds_alternative<double>(args[0]))
            i->setResponseStatusInternal((int)std::get<double>(args[0]));
        return false;
    });
    variables_["setResponseContentType"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        if (!args.empty() && std::holds_alternative<std::string>(args[0]))
            i->setResponseContentTypeInternal(std::get<std::string>(args[0]));
        return false;
    });
    variables_["getRequestHeader"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0])) return std::string("");
        std::string name = std::get<std::string>(args[0]);
        const std::string& headers = i->getCurrentRequestHeaders();
        std::string lowerName = name;
        for (auto& c : lowerName) c = (char)std::tolower((unsigned char)c);
        size_t pos = 0;
        while (pos < headers.size()) {
            size_t lineEnd = headers.find("\r\n", pos);
            if (lineEnd == std::string::npos) lineEnd = headers.find('\n', pos);
            if (lineEnd == std::string::npos) lineEnd = headers.size();
            std::string line = headers.substr(pos, lineEnd - pos);
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                std::string hName = line.substr(0, colon);
                for (auto& c : hName) c = (char)std::tolower((unsigned char)c);
                while (hName.size() && (hName.back() == ' ' || hName.back() == '\t')) hName.pop_back();
                if (hName == lowerName) {
                    std::string val = line.substr(colon + 1);
                    size_t start = val.find_first_not_of(" \t");
                    if (start != std::string::npos) val = val.substr(start);
                    return val;
                }
            }
            pos = lineEnd + (headers.substr(lineEnd, 2) == "\r\n" ? 2 : 1);
        }
        return std::string("");
    });
    variables_["setResponseHeader"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        if (args.size() < 2 || !std::holds_alternative<std::string>(args[0]) || !std::holds_alternative<std::string>(args[1]))
            return false;
        i->addResponseHeader(std::get<std::string>(args[0]), std::get<std::string>(args[1]));
        return false;
    });
    variables_["servePublic"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0])) return false;
        return i->servePublic(std::get<std::string>(args[0]));
    });
    variables_["setHandler"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        if (!args.empty() && std::holds_alternative<std::string>(args[0]))
            i->handlerClassName_ = std::get<std::string>(args[0]);
        return false;
    });
    variables_["listen"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        int port = 8080;
        if (!args.empty() && std::holds_alternative<double>(args[0]))
            port = (int)std::get<double>(args[0]);
        runHttpServer(i, port);
        return false;
    });
    // Stdio / MCP transport
    variables_["readStdinLine"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        (void)args;
        std::string line;
        if (std::getline(std::cin, line))
            return line;
        return std::string("");
    });
    variables_["writeStdout"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (!args.empty() && std::holds_alternative<std::string>(args[0])) {
            std::cout << std::get<std::string>(args[0]) << std::flush;
        }
        return false;
    });
    variables_["writeStderr"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (!args.empty() && std::holds_alternative<std::string>(args[0])) {
            std::cerr << std::get<std::string>(args[0]) << std::flush;
        }
        return false;
    });
    variables_["setMcpRequest"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        if (!args.empty() && std::holds_alternative<std::string>(args[0]))
            i->setMcpRequest(std::get<std::string>(args[0]));
        return false;
    });
    variables_["getMcpRequest"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)args;
        return i->getMcpRequest();
    });
    variables_["setMcpResponse"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        if (!args.empty() && std::holds_alternative<std::string>(args[0]))
            i->setMcpResponse(std::get<std::string>(args[0]));
        return false;
    });
    variables_["getMcpResponse"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)args;
        return i->getMcpResponse();
    });
    variables_["setMcpHandler"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        if (!args.empty() && std::holds_alternative<std::string>(args[0]))
            i->setMcpHandlerClassName(std::get<std::string>(args[0]));
        return false;
    });
    variables_["runMcp"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)args;
        for (;;) {
            std::string line;
            if (!std::getline(std::cin, line)) break;
            if (std::cin.eof() && line.empty()) break;
            i->setMcpRequest(line);
            i->callMcpHandler();
            std::cout << i->getMcpResponse() << "\n" << std::flush;
        }
        return false;
    });
    variables_["readFile"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0])) return std::string("");
        std::string path = std::get<std::string>(args[0]);
        std::string toOpen = i->getResolvedPath(path);
        std::ifstream f(toOpen);
        if (!f) return std::string("");
        std::stringstream ss;
        ss << f.rdbuf();
        return ss.str();
    });
    variables_["writeFile"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (args.size() < 2) return false;
        std::string path = std::holds_alternative<std::string>(args[0]) ? std::get<std::string>(args[0]) : "";
        std::string content = std::holds_alternative<std::string>(args[1]) ? std::get<std::string>(args[1]) : "";
        std::ofstream f(path);
        if (!f) return false;
        f << content;
        return true;
    });
    variables_["arrayCreate"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        (void)args;
        return std::make_shared<MeltArray>();
    });
    variables_["arrayPush"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (args.size() < 2) return false;
        auto* arr = std::get_if<std::shared_ptr<MeltArray>>(&args[0]);
        if (!arr || !*arr) return false;
        (*arr)->data.push_back(args[1]);
        return (double)(*arr)->data.size();
    });
    variables_["arrayGet"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (args.size() < 2) return false;
        auto* arr = std::get_if<std::shared_ptr<MeltArray>>(&args[0]);
        if (!arr || !*arr) return false;
        size_t idx = std::holds_alternative<double>(args[1]) ? (size_t)std::get<double>(args[1]) : 0;
        if (idx >= (*arr)->data.size()) return false;
        return (*arr)->data[idx];
    });
    variables_["arraySet"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (args.size() < 3) return false;
        auto* arr = std::get_if<std::shared_ptr<MeltArray>>(&args[0]);
        if (!arr || !*arr) return false;
        size_t idx = std::holds_alternative<double>(args[1]) ? (size_t)std::get<double>(args[1]) : 0;
        if (idx >= (*arr)->data.size()) (*arr)->data.resize(idx + 1);
        (*arr)->data[idx] = args[2];
        return true;
    });
    variables_["arrayLength"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (args.empty()) return (double)0;
        auto* arr = std::get_if<std::shared_ptr<MeltArray>>(&args[0]);
        if (!arr || !*arr) return (double)0;
        return (double)(*arr)->data.size();
    });
    // Number: format, random, round, floor, ceil, abs, min, max
    variables_["numberFormat"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (args.empty() || !std::holds_alternative<double>(args[0])) return std::string("");
        double n = std::get<double>(args[0]);
        int decimals = 0;
        if (args.size() >= 2 && std::holds_alternative<double>(args[1]))
            decimals = (int)std::get<double>(args[1]);
        if (decimals < 0) decimals = 0;
        if (decimals > 20) decimals = 20;
        char buf[64];
        if (decimals == 0)
            snprintf(buf, sizeof(buf), "%.0f", n);
        else
            snprintf(buf, sizeof(buf), "%.*f", decimals, n);
        return std::string(buf);
    });
    variables_["random"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        static std::mt19937 gen(static_cast<unsigned>(std::random_device{}()));
        if (args.empty()) {
            std::uniform_real_distribution<double> dist(0.0, 1.0);
            return dist(gen);
        }
        if (args.size() >= 2 && std::holds_alternative<double>(args[0]) && std::holds_alternative<double>(args[1])) {
            double lo = std::get<double>(args[0]);
            double hi = std::get<double>(args[1]);
            if (lo > hi) std::swap(lo, hi);
            std::uniform_real_distribution<double> dist(lo, hi);
            return dist(gen);
        }
        return 0.0;
    });
    variables_["randomInt"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        static std::mt19937 gen(static_cast<unsigned>(std::random_device{}()));
        if (args.size() < 2 || !std::holds_alternative<double>(args[0]) || !std::holds_alternative<double>(args[1]))
            return 0.0;
        int lo = (int)std::get<double>(args[0]);
        int hi = (int)std::get<double>(args[1]);
        if (lo > hi) std::swap(lo, hi);
        std::uniform_int_distribution<int> dist(lo, hi);
        return (double)dist(gen);
    });
    variables_["round"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (args.empty() || !std::holds_alternative<double>(args[0])) return 0.0;
        return std::round(std::get<double>(args[0]));
    });
    variables_["floor"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (args.empty() || !std::holds_alternative<double>(args[0])) return 0.0;
        return std::floor(std::get<double>(args[0]));
    });
    variables_["ceil"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (args.empty() || !std::holds_alternative<double>(args[0])) return 0.0;
        return std::ceil(std::get<double>(args[0]));
    });
    variables_["abs"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (args.empty() || !std::holds_alternative<double>(args[0])) return 0.0;
        return std::fabs(std::get<double>(args[0]));
    });
    variables_["min"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (args.size() < 2 || !std::holds_alternative<double>(args[0]) || !std::holds_alternative<double>(args[1]))
            return 0.0;
        double a = std::get<double>(args[0]), b = std::get<double>(args[1]);
        return a < b ? a : b;
    });
    variables_["max"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (args.size() < 2 || !std::holds_alternative<double>(args[0]) || !std::holds_alternative<double>(args[1]))
            return 0.0;
        double a = std::get<double>(args[0]), b = std::get<double>(args[1]);
        return a > b ? a : b;
    });
    variables_["addModulePath"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0])) return std::monostate{};
        std::string dir = std::get<std::string>(args[0]);
        if (dir.empty()) return std::monostate{};
        std::string full = (i->getResolvedPath(dir));
        i->addModulePath(full);
        return std::monostate{};
    });
    variables_["getConfig"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0])) return std::string("");
        return i->getConfig(std::get<std::string>(args[0]));
    });
    variables_["chr"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (args.empty() || !std::holds_alternative<double>(args[0])) return std::string("");
        int n = (int)std::get<double>(args[0]);
        if (n < 0 || n > 255) return std::string("");
        return std::string(1, (char)n);
    });
    variables_["splitString"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (args.size() < 2 || !std::holds_alternative<std::string>(args[0]) || !std::holds_alternative<std::string>(args[1]))
            return std::make_shared<MeltArray>();
        std::string s = std::get<std::string>(args[0]);
        std::string sep = std::get<std::string>(args[1]);
        auto arr = std::make_shared<MeltArray>();
        if (sep.empty()) {
            for (unsigned char c : s) arr->data.push_back(std::string(1, (char)c));
            return arr;
        }
        size_t pos = 0;
        for (;;) {
            size_t next = s.find(sep, pos);
            if (next == std::string::npos) {
                arr->data.push_back(s.substr(pos));
                break;
            }
            arr->data.push_back(s.substr(pos, next - pos));
            pos = next + sep.size();
        }
        return arr;
    });
    variables_["escapeHtml"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (args.empty() || !std::holds_alternative<std::string>(args[0])) return std::string("");
        std::string s = std::get<std::string>(args[0]);
        std::string out;
        out.reserve(s.size());
        for (char c : s) {
            if (c == '&') out += "&amp;";
            else if (c == '<') out += "&lt;";
            else if (c == '>') out += "&gt;";
            else if (c == '"') out += "&quot;";
            else out += c;
        }
        return out;
    });
    variables_["replaceString"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (args.size() < 3 || !std::holds_alternative<std::string>(args[0]) ||
            !std::holds_alternative<std::string>(args[1]) || !std::holds_alternative<std::string>(args[2]))
            return std::string("");
        std::string s = std::get<std::string>(args[0]);
        std::string from = std::get<std::string>(args[1]);
        std::string to = std::get<std::string>(args[2]);
        if (from.empty()) return s;
        std::string out;
        size_t pos = 0;
        while (pos < s.size()) {
            size_t next = s.find(from, pos);
            if (next == std::string::npos) { out += s.substr(pos); break; }
            out += s.substr(pos, next - pos);
            out += to;
            pos = next + from.size();
        }
        return out;
    });
    variables_["urlDecode"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (args.empty() || !std::holds_alternative<std::string>(args[0])) return std::string("");
        std::string s = std::get<std::string>(args[0]);
        std::string out;
        out.reserve(s.size());
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '+') { out += ' '; continue; }
            if (s[i] == '%' && i + 2 < s.size()) {
                int hi = (s[i+1] >= 'A' ? (s[i+1] & 0x5F) - 'A' + 10 : (s[i+1] <= '9' ? s[i+1] - '0' : -1));
                int lo = (s[i+2] >= 'A' ? (s[i+2] & 0x5F) - 'A' + 10 : (s[i+2] <= '9' ? s[i+2] - '0' : -1));
                if (hi >= 0 && hi <= 15 && lo >= 0 && lo <= 15) {
                    out += (char)((hi << 4) + lo);
                    i += 2;
                    continue;
                }
            }
            out += s[i];
        }
        return out;
    });
    variables_["jsonEncode"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (args.empty()) return std::string("null");
        return valueToJson(args[0]);
    });
    variables_["jsonDecode"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0])) return false;
        std::string s = std::get<std::string>(args[0]);
        const char* p = s.c_str();
        const char* end = p + s.size();
        return parseJsonValue(i, p, end);
    });
    variables_["objectCreate"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)args;
        auto obj = std::make_shared<MeltObject>();
        obj->klass = i->getJsonObjectClass();
        return obj;
    });
    // Vector math (2D and 3D)
    auto vecFromValue = [](const Value& v) -> std::shared_ptr<MeltVec> {
        auto* p = std::get_if<std::shared_ptr<MeltVec>>(&v);
        if (!p || !*p) throw std::runtime_error("Expected vector");
        return *p;
    };
    variables_["vectorCreate2"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        auto v = std::make_shared<MeltVec>();
        v->dim = 2;
        v->x = (args.size() > 0 && std::holds_alternative<double>(args[0])) ? std::get<double>(args[0]) : 0;
        v->y = (args.size() > 1 && std::holds_alternative<double>(args[1])) ? std::get<double>(args[1]) : 0;
        return v;
    });
    variables_["vectorCreate3"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        auto v = std::make_shared<MeltVec>();
        v->dim = 3;
        v->x = (args.size() > 0 && std::holds_alternative<double>(args[0])) ? std::get<double>(args[0]) : 0;
        v->y = (args.size() > 1 && std::holds_alternative<double>(args[1])) ? std::get<double>(args[1]) : 0;
        v->z = (args.size() > 2 && std::holds_alternative<double>(args[2])) ? std::get<double>(args[2]) : 0;
        return v;
    });
    variables_["vectorAdd"] = reg([vecFromValue](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (args.size() < 2) throw std::runtime_error("vectorAdd requires two vectors");
        auto a = vecFromValue(args[0]);
        auto b = vecFromValue(args[1]);
        if (a->dim != b->dim) throw std::runtime_error("vectorAdd: vectors must have same dimension");
        auto r = std::make_shared<MeltVec>();
        r->dim = a->dim;
        r->x = a->x + b->x;
        r->y = a->y + b->y;
        r->z = a->z + b->z;
        return r;
    });
    variables_["vectorSub"] = reg([vecFromValue](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (args.size() < 2) throw std::runtime_error("vectorSub requires two vectors");
        auto a = vecFromValue(args[0]);
        auto b = vecFromValue(args[1]);
        if (a->dim != b->dim) throw std::runtime_error("vectorSub: vectors must have same dimension");
        auto r = std::make_shared<MeltVec>();
        r->dim = a->dim;
        r->x = a->x - b->x;
        r->y = a->y - b->y;
        r->z = a->z - b->z;
        return r;
    });
    variables_["vectorScale"] = reg([vecFromValue](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (args.size() < 2) throw std::runtime_error("vectorScale requires vector and number");
        auto v = vecFromValue(args[0]);
        double s = (std::holds_alternative<double>(args[1])) ? std::get<double>(args[1]) : 0;
        auto r = std::make_shared<MeltVec>();
        r->dim = v->dim;
        r->x = v->x * s;
        r->y = v->y * s;
        r->z = v->z * s;
        return r;
    });
    variables_["vectorLength"] = reg([vecFromValue](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (args.empty()) throw std::runtime_error("vectorLength requires a vector");
        auto v = vecFromValue(args[0]);
        double len = std::sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
        return len;
    });
    variables_["vectorDot"] = reg([vecFromValue](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (args.size() < 2) throw std::runtime_error("vectorDot requires two vectors");
        auto a = vecFromValue(args[0]);
        auto b = vecFromValue(args[1]);
        if (a->dim != b->dim) throw std::runtime_error("vectorDot: vectors must have same dimension");
        return a->x * b->x + a->y * b->y + a->z * b->z;
    });
    variables_["vectorCross"] = reg([vecFromValue](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (args.size() < 2) throw std::runtime_error("vectorCross requires two 3D vectors");
        auto a = vecFromValue(args[0]);
        auto b = vecFromValue(args[1]);
        if (a->dim != 3 || b->dim != 3) throw std::runtime_error("vectorCross requires 3D vectors");
        auto r = std::make_shared<MeltVec>();
        r->dim = 3;
        r->x = a->y * b->z - a->z * b->y;
        r->y = a->z * b->x - a->x * b->z;
        r->z = a->x * b->y - a->y * b->x;
        return r;
    });
    variables_["vectorX"] = reg([vecFromValue](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (args.empty()) throw std::runtime_error("vectorX requires a vector");
        return vecFromValue(args[0])->x;
    });
    variables_["vectorY"] = reg([vecFromValue](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (args.empty()) throw std::runtime_error("vectorY requires a vector");
        return vecFromValue(args[0])->y;
    });
    variables_["vectorZ"] = reg([vecFromValue](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (args.empty()) throw std::runtime_error("vectorZ requires a vector");
        return vecFromValue(args[0])->z;
    });
    variables_["vectorDim"] = reg([vecFromValue](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (args.empty()) throw std::runtime_error("vectorDim requires a vector");
        return (double)vecFromValue(args[0])->dim;
    });
    // GUI render: image buffer (RGB 0-255), then save as PPM
    variables_["imageCreate"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        int w = args.size() > 0 && std::holds_alternative<double>(args[0]) ? (int)std::get<double>(args[0]) : 0;
        int h = args.size() > 1 && std::holds_alternative<double>(args[1]) ? (int)std::get<double>(args[1]) : 0;
        if (w <= 0 || h <= 0 || w > 8192 || h > 8192) throw std::runtime_error("imageCreate: width and height must be 1..8192");
        i->imageWidth_ = w;
        i->imageHeight_ = h;
        i->imageData_.assign((size_t)(w * h * 3), 0);
        return true;
    });
    variables_["imageFill"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        if (i->imageData_.empty()) throw std::runtime_error("No image; call imageCreate first");
        int r = 0, g = 0, b = 0;
        if (args.size() >= 3 && std::holds_alternative<double>(args[0]) && std::holds_alternative<double>(args[1]) && std::holds_alternative<double>(args[2])) {
            r = (int)std::get<double>(args[0]) & 255;
            g = (int)std::get<double>(args[1]) & 255;
            b = (int)std::get<double>(args[2]) & 255;
        } else if (args.size() >= 1 && std::holds_alternative<double>(args[0])) {
            r = g = b = (int)std::get<double>(args[0]) & 255;
        }
        for (size_t j = 0; j < i->imageData_.size(); j += 3) {
            i->imageData_[j] = (uint8_t)r;
            i->imageData_[j + 1] = (uint8_t)g;
            i->imageData_[j + 2] = (uint8_t)b;
        }
        return true;
    });
    variables_["imageSetPixel"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        if (i->imageData_.empty()) throw std::runtime_error("No image; call imageCreate first");
        if (args.size() < 2) return false;
        int x = (int)std::get<double>(args[0]);
        int y = (int)std::get<double>(args[1]);
        int r = 255, g = 255, b = 255;
        if (args.size() >= 5) {
            r = (int)std::get<double>(args[2]) & 255;
            g = (int)std::get<double>(args[3]) & 255;
            b = (int)std::get<double>(args[4]) & 255;
        } else if (args.size() >= 3 && std::holds_alternative<double>(args[2])) {
            r = g = b = (int)std::get<double>(args[2]) & 255;
        }
        if (x >= 0 && x < i->imageWidth_ && y >= 0 && y < i->imageHeight_) {
            size_t idx = (size_t)(y * i->imageWidth_ + x) * 3;
            i->imageData_[idx] = (uint8_t)r;
            i->imageData_[idx + 1] = (uint8_t)g;
            i->imageData_[idx + 2] = (uint8_t)b;
        }
        return true;
    });
    variables_["imageDrawLine"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        if (i->imageData_.empty()) throw std::runtime_error("No image; call imageCreate first");
        if (args.size() < 4) throw std::runtime_error("imageDrawLine requires x1, y1, x2, y2");
        int x1 = (int)std::get<double>(args[0]);
        int y1 = (int)std::get<double>(args[1]);
        int x2 = (int)std::get<double>(args[2]);
        int y2 = (int)std::get<double>(args[3]);
        int r = args.size() > 4 ? ((int)std::get<double>(args[4]) & 255) : 255;
        int g = args.size() > 5 ? ((int)std::get<double>(args[5]) & 255) : 255;
        int b = args.size() > 6 ? ((int)std::get<double>(args[6]) & 255) : 255;
        imageDrawLineBresenham(i->imageData_, i->imageWidth_, i->imageHeight_, x1, y1, x2, y2, (uint8_t)r, (uint8_t)g, (uint8_t)b);
        return true;
    });
    variables_["imageSavePpm"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0])) return false;
        std::string path = std::get<std::string>(args[0]);
        std::string full = i->getResolvedPath(path);
        return i->saveImagePpm(full);
    });
#ifdef USE_GUI
    variables_["imagePreview"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)args;
        if (i->imageData_.empty()) throw std::runtime_error("No image; call imageCreate first");
        runImagePreviewWindow(i->imageWidth_, i->imageHeight_, i->imageData_.data());
        return false;
    });
#else
    variables_["imagePreview"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        (void)args;
        throw std::runtime_error("Melt was not built with GUI support. Use: make with-gui (requires SDL2)");
    });
#endif
    variables_["base64Encode"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (args.empty() || !std::holds_alternative<std::string>(args[0])) return std::string("");
        return base64Encode(std::get<std::string>(args[0]));
    });
    variables_["base64Decode"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (args.empty() || !std::holds_alternative<std::string>(args[0])) return std::string("");
        return base64Decode(std::get<std::string>(args[0]));
    });
    variables_["xorCipher"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (args.size() < 2) return std::string("");
        std::string data = std::holds_alternative<std::string>(args[0]) ? std::get<std::string>(args[0]) : "";
        std::string key = std::holds_alternative<std::string>(args[1]) ? std::get<std::string>(args[1]) : "";
        return xorCipher(data, key);
    });
    variables_["renderView"] = reg([](Interpreter* i, std::vector<Value> args) -> Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0])) return std::string("");
        std::string path = std::get<std::string>(args[0]);
        std::shared_ptr<MeltObject> obj;
        if (args.size() >= 2 && std::holds_alternative<std::shared_ptr<MeltObject>>(args[1]))
            obj = std::get<std::shared_ptr<MeltObject>>(args[1]);
        return i->renderViewTemplate(path, obj);
    });
    registerMysqlBuiltins(this);
}

void Interpreter::registerBuiltin(const std::string& name, NativeFn fn) {
    size_t i = nativeFunctions_.size();
    nativeFunctions_.push_back(std::move(fn));
    variables_[name] = NativeFunc{i};
}

std::shared_ptr<MeltClass> Interpreter::getJsonObjectClass() {
    if (!jsonObjectClass_) {
        jsonObjectClass_ = std::make_shared<MeltClass>();
        jsonObjectClass_->name = "JsonObject";
    }
    return jsonObjectClass_;
}

void Interpreter::execute(Stmt& stmt) {
    if (auto p = dynamic_cast<PrintStmt*>(&stmt)) { executePrint(*p); return; }
    if (auto p = dynamic_cast<ExprStmt*>(&stmt)) { executeExprStmt(*p); return; }
    if (auto p = dynamic_cast<ImportStmt*>(&stmt)) { executeImport(*p); return; }
    if (auto p = dynamic_cast<LetStmt*>(&stmt)) { executeLet(*p); return; }
    if (auto p = dynamic_cast<AssignStmt*>(&stmt)) { executeAssign(*p); return; }
    if (auto p = dynamic_cast<ClassDeclStmt*>(&stmt)) { executeClass(*p); return; }
    if (auto p = dynamic_cast<SetPropertyStmt*>(&stmt)) { executeSetProperty(*p); return; }
    if (auto p = dynamic_cast<SetIndexStmt*>(&stmt)) { executeSetIndex(*p); return; }
    if (auto p = dynamic_cast<BlockStmt*>(&stmt)) { executeBlock(*p); return; }
    if (auto p = dynamic_cast<IfStmt*>(&stmt)) { executeIf(*p); return; }
    if (auto p = dynamic_cast<WhileStmt*>(&stmt)) { executeWhile(*p); return; }
    if (auto p = dynamic_cast<ReturnStmt*>(&stmt)) { executeReturn(*p); return; }
    if (auto p = dynamic_cast<TryCatchStmt*>(&stmt)) { executeTryCatch(*p); return; }
    if (auto p = dynamic_cast<ThrowStmt*>(&stmt)) { executeThrow(*p); return; }
}

Value Interpreter::evaluate(const Expr& expr) {
    if (auto p = dynamic_cast<const NumberExpr*>(&expr)) return evaluateNumber(*p);
    if (auto p = dynamic_cast<const StringExpr*>(&expr)) return evaluateString(*p);
    if (auto p = dynamic_cast<const BoolExpr*>(&expr)) return evaluateBool(*p);
    if (auto p = dynamic_cast<const VarExpr*>(&expr)) return evaluateVar(*p);
    if (auto p = dynamic_cast<const ThisExpr*>(&expr)) return evaluateThis(*p);
    if (auto p = dynamic_cast<const GetExpr*>(&expr)) return evaluateGet(*p);
    if (auto p = dynamic_cast<const CallExpr*>(&expr)) return evaluateCall(*p);
    if (auto p = dynamic_cast<const ArrayExpr*>(&expr)) return evaluateArray(*p);
    if (auto p = dynamic_cast<const IndexExpr*>(&expr)) return evaluateIndex(*p);
    if (auto p = dynamic_cast<const UnaryExpr*>(&expr)) return evaluateUnary(*p);
    if (auto p = dynamic_cast<const BinaryExpr*>(&expr)) return evaluateBinary(*p);
    return false;
}

void Interpreter::executePrint(const PrintStmt& stmt) {
    Value v = evaluate(*stmt.expr);
    printValue(v);
    std::cout << "\n";
}

void Interpreter::executeExprStmt(const ExprStmt& stmt) {
    evaluate(*stmt.expr);
}

void Interpreter::executeImport(const ImportStmt& stmt) {
    std::string resolved = resolveImportPath(stmt.path);
    if (importedPaths_.count(resolved)) return;
    importedPaths_.insert(resolved);
    std::string source = readFile(resolved);
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens));
    auto parsed = parser.parse();
    std::string prevDir = currentDir_;
    currentDir_ = dirname(resolved);
    for (const auto& s : parsed) execute(*s);
    currentDir_ = prevDir;
}

void Interpreter::executeLet(const LetStmt& stmt) {
    variables_[stmt.name] = evaluate(*stmt.expr);
}

void Interpreter::executeClass(ClassDeclStmt& stmt) {
    auto klass = std::make_shared<MeltClass>();
    klass->name = stmt.name;
    for (auto& m : stmt.methods) {
        MeltMethod mm;
        mm.params = m.params;
        mm.body = std::move(m.body);
        klass->methods[m.name] = std::move(mm);
    }
    variables_[stmt.name] = klass;
}

void Interpreter::executeSetProperty(const SetPropertyStmt& stmt) {
    Value objVal = evaluate(*stmt.object);
    auto* obj = std::get_if<std::shared_ptr<MeltObject>>(&objVal);
    if (!obj || !*obj)
        throw std::runtime_error("Expected object for property assignment");
    setField(**obj, stmt.name, evaluate(*stmt.value));
}

void Interpreter::executeSetIndex(const SetIndexStmt& stmt) {
    Value baseVal = evaluate(*stmt.array);
    Value idxVal = evaluate(*stmt.index);
    if (auto* obj = std::get_if<std::shared_ptr<MeltObject>>(&baseVal)) {
        if (obj->get()) {
            if (!std::holds_alternative<std::string>(idxVal))
                throw std::runtime_error("Object index must be string");
            setField(**obj, std::get<std::string>(idxVal), evaluate(*stmt.value));
            return;
        }
    }
    auto* arr = std::get_if<std::shared_ptr<MeltArray>>(&baseVal);
    if (!arr || !*arr)
        throw std::runtime_error("Expected array or object for index assignment");
    size_t i = 0;
    if (std::holds_alternative<double>(idxVal)) i = (size_t)std::get<double>(idxVal);
    if (i >= (*arr)->data.size()) (*arr)->data.resize(i + 1);
    (*arr)->data[i] = evaluate(*stmt.value);
}

void Interpreter::executeAssign(const AssignStmt& stmt) {
    if (variables_.find(stmt.name) == variables_.end())
        throw std::runtime_error("Unknown variable: " + stmt.name);
    variables_[stmt.name] = evaluate(*stmt.expr);
}

void Interpreter::executeBlock(const BlockStmt& stmt) {
    for (const auto& s : stmt.statements) execute(*s);
}

void Interpreter::executeIf(const IfStmt& stmt) {
    if (isTruthy(evaluate(*stmt.condition)))
        execute(*stmt.thenBranch);
    else if (stmt.elseBranch)
        execute(*stmt.elseBranch);
}

void Interpreter::executeWhile(const WhileStmt& stmt) {
    while (isTruthy(evaluate(*stmt.condition)))
        execute(*stmt.body);
}

void Interpreter::executeReturn(const ReturnStmt& stmt) {
    Value v = stmt.value ? evaluate(*stmt.value) : Value{std::monostate{}};
    throw ReturnException(std::move(v));
}

void Interpreter::executeTryCatch(const TryCatchStmt& stmt) {
    try {
        for (const auto& s : stmt.tryBody->statements)
            execute(*s);
    } catch (const ThrowException& e) {
        variables_[stmt.catchVar] = e.value;
        for (const auto& s : stmt.catchBody->statements)
            execute(*s);
    } catch (const ReturnException&) {
        throw;  // rethrow so method return propagates
    } catch (const std::exception& e) {
        variables_[stmt.catchVar] = std::string(e.what());
        for (const auto& s : stmt.catchBody->statements)
            execute(*s);
    }
}

void Interpreter::executeThrow(const ThrowStmt& stmt) {
    Value v = evaluate(*stmt.value);
    throw ThrowException(std::move(v));
}

Value Interpreter::evaluateNumber(const NumberExpr& expr) {
    return expr.value;
}

Value Interpreter::evaluateString(const StringExpr& expr) {
    return expr.value;
}

Value Interpreter::evaluateBool(const BoolExpr& expr) {
    return expr.value;
}

Value Interpreter::evaluateVar(const VarExpr& expr) {
    auto it = variables_.find(expr.name);
    if (it != variables_.end()) return it->second;
    if (std::holds_alternative<std::shared_ptr<MeltObject>>(this_)) {
        auto& obj = *std::get<std::shared_ptr<MeltObject>>(this_);
        auto fit = obj.fields.find(expr.name);
        if (fit != obj.fields.end()) return fit->second;
    }
    throw std::runtime_error("Unknown variable: " + expr.name);
}

Value Interpreter::evaluateThis(const ThisExpr&) {
    if (!std::holds_alternative<std::shared_ptr<MeltObject>>(this_))
        throw std::runtime_error("'this' used outside of method");
    return this_;
}

Value Interpreter::evaluateGet(const GetExpr& expr) {
    Value objVal = evaluate(*expr.object);
    if (auto* obj = std::get_if<std::shared_ptr<MeltObject>>(&objVal))
        return getField(*obj, expr.name);
    throw std::runtime_error("Expected object for property access");
}

Value Interpreter::evaluateCall(const CallExpr& expr) {
    Value callee = evaluate(*expr.callee);

    if (auto* klass = std::get_if<std::shared_ptr<MeltClass>>(&callee)) {
        auto obj = std::make_shared<MeltObject>();
        obj->klass = *klass;
        Value prevThis = this_;
        this_ = obj;
        auto it = (*klass)->methods.find("init");
        if (it != (*klass)->methods.end()) {
            const MeltMethod& init = it->second;
            if (init.params.size() != expr.args.size())
                throw std::runtime_error("init argument count mismatch");
            std::vector<std::string> paramNames = init.params;
            for (size_t i = 0; i < paramNames.size(); ++i) {
                Value argVal = evaluate(*expr.args[i]);
                obj->fields[paramNames[i]] = argVal;
                variables_[paramNames[i]] = argVal;
            }
            try {
                for (const auto& s : init.body->statements) execute(*s);
            } catch (const ReturnException&) { /* init may return early; ignore */ }
            for (const auto& p : paramNames) variables_.erase(p);
        }
        this_ = prevThis;
        return obj;
    }

    if (auto* bm = std::get_if<BoundMethod>(&callee)) {
        Value prevThis = this_;
        this_ = bm->receiver;
        auto it = bm->receiver->klass->methods.find(bm->methodName);
        if (it == bm->receiver->klass->methods.end())
            throw std::runtime_error("Method not found: " + bm->methodName);
        const MeltMethod& m = it->second;
        if (m.params.size() != expr.args.size())
            throw std::runtime_error("Argument count mismatch for " + bm->methodName);
        std::vector<std::string> paramNames = m.params;
        for (size_t i = 0; i < paramNames.size(); ++i)
            variables_[paramNames[i]] = evaluate(*expr.args[i]);
        Value result(std::monostate{});
        try {
            for (const auto& s : m.body->statements) execute(*s);
        } catch (const ReturnException& e) {
            result = e.value;
        }
        for (const auto& p : paramNames) variables_.erase(p);
        this_ = prevThis;
        return result;
    }

    if (auto* nf = std::get_if<NativeFunc>(&callee)) {
        std::vector<Value> args;
        for (const auto& a : expr.args) args.push_back(evaluate(*a));
        if (nf->index >= nativeFunctions_.size())
            throw std::runtime_error("Invalid native function");
        return nativeFunctions_[nf->index](this, std::move(args));
    }

    throw std::runtime_error("Can only call class constructor, method, or built-in");
}

Value Interpreter::getField(std::shared_ptr<MeltObject> obj, const std::string& name) {
    auto it = obj->fields.find(name);
    if (it != obj->fields.end()) return it->second;
    auto mit = obj->klass->methods.find(name);
    if (mit != obj->klass->methods.end())
        return BoundMethod{obj, name};
    throw std::runtime_error("Unknown property: " + name);
}

void Interpreter::setField(MeltObject& obj, const std::string& name, Value v) {
    obj.fields[name] = std::move(v);
}

Value Interpreter::evaluateArray(const ArrayExpr& expr) {
    auto arr = std::make_shared<MeltArray>();
    for (const auto& e : expr.elements) arr->data.push_back(evaluate(*e));
    return arr;
}

Value Interpreter::evaluateIndex(const IndexExpr& expr) {
    Value baseVal = evaluate(*expr.array);
    Value idxVal = evaluate(*expr.index);
    if (auto* obj = std::get_if<std::shared_ptr<MeltObject>>(&baseVal)) {
        if (obj->get()) {
            if (!std::holds_alternative<std::string>(idxVal))
                throw std::runtime_error("Object index must be string");
            const std::string& key = std::get<std::string>(idxVal);
            auto it = (*obj)->fields.find(key);
            if (it != (*obj)->fields.end()) return it->second;
            return false;
        }
    }
    auto* arr = std::get_if<std::shared_ptr<MeltArray>>(&baseVal);
    if (!arr || !*arr) throw std::runtime_error("Expected array or object for index");
    size_t i = std::holds_alternative<double>(idxVal) ? (size_t)std::get<double>(idxVal) : 0;
    if (i >= (*arr)->data.size()) throw std::runtime_error("Array index out of range");
    return (*arr)->data[i];
}

Value Interpreter::evaluateUnary(const UnaryExpr& expr) {
    if (expr.op == "!") {
        return !isTruthy(evaluate(*expr.operand));
    }
    return false;
}

Value Interpreter::evaluateBinary(const BinaryExpr& expr) {
    if (expr.op == "&&") {
        Value left = evaluate(*expr.left);
        if (!isTruthy(left)) return left;
        return evaluate(*expr.right);
    }
    if (expr.op == "||") {
        Value left = evaluate(*expr.left);
        if (isTruthy(left)) return left;
        return evaluate(*expr.right);
    }

    Value left = evaluate(*expr.left);
    Value right = evaluate(*expr.right);

    if (expr.op == "==") {
        if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right))
            return std::get<double>(left) == std::get<double>(right);
        if (std::holds_alternative<std::string>(left) && std::holds_alternative<std::string>(right))
            return std::get<std::string>(left) == std::get<std::string>(right);
        return isTruthy(left) == isTruthy(right);
    }
    if (expr.op == "!=") {
        if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right))
            return std::get<double>(left) != std::get<double>(right);
        if (std::holds_alternative<std::string>(left) && std::holds_alternative<std::string>(right))
            return std::get<std::string>(left) != std::get<std::string>(right);
        return isTruthy(left) != isTruthy(right);
    }

    double l = 0, r = 0;
    if (std::holds_alternative<double>(left)) l = std::get<double>(left);
    if (std::holds_alternative<double>(right)) r = std::get<double>(right);

    if (expr.op == "+") {
        if (std::holds_alternative<std::string>(left) && std::holds_alternative<std::string>(right))
            return std::get<std::string>(left) + std::get<std::string>(right);
        if (std::holds_alternative<std::string>(left))
            return std::get<std::string>(left) + std::to_string(static_cast<long long>(r));
        if (std::holds_alternative<std::string>(right))
            return std::to_string(static_cast<long long>(l)) + std::get<std::string>(right);
        return l + r;
    }
    if (expr.op == "-") return l - r;
    if (expr.op == "*") return l * r;
    if (expr.op == "/") return l / r;
    if (expr.op == "<") return l < r;
    if (expr.op == "<=") return l <= r;
    if (expr.op == ">") return l > r;
    if (expr.op == ">=") return l >= r;

    return false;
}

bool Interpreter::isTruthy(const Value& v) {
    if (std::holds_alternative<std::monostate>(v)) return false;
    if (std::holds_alternative<bool>(v)) return std::get<bool>(v);
    if (std::holds_alternative<double>(v)) return std::get<double>(v) != 0;
    if (std::holds_alternative<std::string>(v)) return !std::get<std::string>(v).empty();
    if (std::holds_alternative<std::shared_ptr<MeltArray>>(v)) return !std::get<std::shared_ptr<MeltArray>>(v)->data.empty();
    if (std::holds_alternative<std::shared_ptr<MeltVec>>(v)) return true;
    return false;
}

void Interpreter::printValue(const Value& v) {
    if (std::holds_alternative<std::monostate>(v)) return;
    if (std::holds_alternative<double>(v)) {
        double d = std::get<double>(v);
        if (d == static_cast<long long>(d))
            std::cout << static_cast<long long>(d);
        else
            std::cout << d;
    } else if (std::holds_alternative<std::string>(v)) {
        std::cout << std::get<std::string>(v);
    } else if (std::holds_alternative<bool>(v)) {
        std::cout << (std::get<bool>(v) ? "true" : "false");
    } else if (std::holds_alternative<std::shared_ptr<MeltClass>>(v)) {
        std::cout << "<class " << std::get<std::shared_ptr<MeltClass>>(v)->name << ">";
    } else if (std::holds_alternative<std::shared_ptr<MeltObject>>(v)) {
        std::cout << "<" << std::get<std::shared_ptr<MeltObject>>(v)->klass->name << " instance>";
    } else if (std::holds_alternative<BoundMethod>(v)) {
        std::cout << "<bound method>";
    } else if (std::holds_alternative<NativeFunc>(v)) {
        std::cout << "<native function>";
    } else if (std::holds_alternative<std::shared_ptr<MeltArray>>(v)) {
        auto& a = *std::get<std::shared_ptr<MeltArray>>(v);
        std::cout << "[";
        for (size_t i = 0; i < a.data.size(); ++i) {
            if (i) std::cout << ", ";
            printValue(a.data[i]);
        }
        std::cout << "]";
    } else if (std::holds_alternative<std::shared_ptr<MeltVec>>(v)) {
        auto& vec = *std::get<std::shared_ptr<MeltVec>>(v);
        if (vec.dim == 2)
            std::cout << "(" << vec.x << ", " << vec.y << ")";
        else
            std::cout << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
    }
}
