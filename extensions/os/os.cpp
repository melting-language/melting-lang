// OS utility extension for Melt.
// Build as shared library in bin/modules and enable with:
//   extension = os

#include "interpreter.hpp"
#include <cstdlib>
#include <filesystem>
#include <string>

static std::string asString(const Value& v) {
    if (auto* s = std::get_if<std::string>(&v)) return *s;
    if (auto* d = std::get_if<double>(&v)) return std::to_string((int)*d);
    if (auto* b = std::get_if<bool>(&v)) return *b ? "true" : "false";
    return "";
}

static Value osVersion(Interpreter*, std::vector<Value>) {
    return Value(std::string("1.0"));
}

static Value osName(Interpreter*, std::vector<Value>) {
#if defined(_WIN32) || defined(_WIN64)
    return Value(std::string("windows"));
#elif defined(_APPLE_)
    return Value(std::string("macos"));
#elif defined(_linux_)
    return Value(std::string("linux"));
#else
    return Value(std::string("unknown"));
#endif
}

static Value osArch(Interpreter*, std::vector<Value>) {
#if defined(__aarch64__) || defined(_M_ARM64)
    return Value(std::string("arm64"));
#elif defined(__x86_64__) || defined(_M_X64)
    return Value(std::string("x64"));
#elif defined(__i386__) || defined(_M_IX86)
    return Value(std::string("x86"));
#else
    return Value(std::string("unknown"));
#endif
}

static Value osPwd(Interpreter*, std::vector<Value>) {
    try {
        return Value(std::filesystem::current_path().generic_string());
    } catch (...) {
        return Value(std::string(""));
    }
}

static Value osGetEnv(Interpreter*, std::vector<Value> args) {
    if (args.empty()) return Value(std::string(""));
    std::string key = asString(args[0]);
    if (key.empty()) return Value(std::string(""));
    const char* val = std::getenv(key.c_str());
    return Value(std::string(val ? val : ""));
}

static Value osExec(Interpreter*, std::vector<Value> args) {
    if (args.empty()) return Value(1.0);
    std::string cmd = asString(args[0]);
    if (cmd.empty()) return Value(1.0);
    int rc = std::system(cmd.c_str());
    return Value((double)rc);
}

static Value osFileExists(Interpreter*, std::vector<Value> args) {
    if (args.empty()) return Value(false);
    std::string path = asString(args[0]);
    if (path.empty()) return Value(false);
    try {
        return Value(std::filesystem::exists(path));
    } catch (...) {
        return Value(false);
    }
}

static Value osMkdirs(Interpreter*, std::vector<Value> args) {
    if (args.empty()) return Value(false);
    std::string path = asString(args[0]);
    if (path.empty()) return Value(false);
    try {
        return Value(std::filesystem::create_directories(path) || std::filesystem::exists(path));
    } catch (...) {
        return Value(false);
    }
}

extern "C" void melt_register(Interpreter* interp) {
    interp->registerBuiltin("osVersion", osVersion);
    interp->registerBuiltin("osName", osName);
    interp->registerBuiltin("osArch", osArch);
    interp->registerBuiltin("osPwd", osPwd);
    interp->registerBuiltin("osGetEnv", osGetEnv);
    interp->registerBuiltin("osExec", osExec);
    interp->registerBuiltin("osFileExists", osFileExists);
    interp->registerBuiltin("osMkdirs", osMkdirs);
}
