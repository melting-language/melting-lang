// Headless browser extension for Melt.
// Build as shared library and place in bin/modules/.
// Enable with: extension = headless_browser in melt.ini or melt.config.

#include "interpreter.hpp"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

static std::string asString(const Value& v) {
    if (auto* s = std::get_if<std::string>(&v)) return *s;
    if (auto* d = std::get_if<double>(&v)) return std::to_string((int)*d);
    if (auto* b = std::get_if<bool>(&v)) return *b ? "true" : "false";
    return "";
}

static int asInt(const Value& v, int def = 0) {
    if (auto* d = std::get_if<double>(&v)) return (int)*d;
    return def;
}

static std::string shellQuote(const std::string& s) {
#if defined(_WIN32) || defined(_WIN64)
    std::string out = "\"";
    for (char c : s) {
        if (c == '"' || c == '\\') out.push_back('\\');
        out.push_back(c);
    }
    out.push_back('"');
    return out;
#else
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') out += "'\\''";
        else out.push_back(c);
    }
    out.push_back('\'');
    return out;
#endif
}

static bool commandExists(const std::string& cmd) {
#if defined(_WIN32) || defined(_WIN64)
    std::string test = "where " + shellQuote(cmd) + " >NUL 2>&1";
#else
    std::string test = "command -v " + shellQuote(cmd) + " >/dev/null 2>&1";
#endif
    return std::system(test.c_str()) == 0;
}

static std::string findBrowserBinary() {
#if defined(__APPLE__)
    const char* macCandidates[] = {
        "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome",
        "/Applications/Chromium.app/Contents/MacOS/Chromium",
        "/Applications/Microsoft Edge.app/Contents/MacOS/Microsoft Edge"
    };
    for (const char* p : macCandidates) {
        if (std::filesystem::exists(p)) return std::string(p);
    }
#endif

    const char* pathCandidates[] = {
        "google-chrome",
        "chromium",
        "chromium-browser",
        "chrome",
        "msedge",
        "microsoft-edge"
    };
    for (const char* c : pathCandidates) {
        if (commandExists(c)) return std::string(c);
    }
    return "";
}

static int runCmd(const std::string& cmd) {
    return std::system(cmd.c_str());
}

static Value headlessBrowserVersion(Interpreter*, std::vector<Value>) {
    return Value(std::string("1.0"));
}

static Value browserAvailable(Interpreter*, std::vector<Value>) {
    return Value(!findBrowserBinary().empty());
}

// browserScreenshot(url, outputPath, width?, height?)
static Value browserScreenshot(Interpreter*, std::vector<Value> args) {
    if (args.size() < 2) return Value(false);
    std::string url = asString(args[0]);
    std::string out = asString(args[1]);
    int width = args.size() > 2 ? asInt(args[2], 1280) : 1280;
    int height = args.size() > 3 ? asInt(args[3], 720) : 720;
    if (url.empty() || out.empty()) return Value(false);
    if (width <= 0) width = 1280;
    if (height <= 0) height = 720;

    std::string bin = findBrowserBinary();
    if (bin.empty()) return Value(false);

#if defined(_WIN32) || defined(_WIN64)
    std::string cmd = shellQuote(bin) + " --headless --disable-gpu --window-size=" +
                      std::to_string(width) + "," + std::to_string(height) +
                      " --screenshot=" + shellQuote(out) + " " + shellQuote(url) + " >NUL 2>&1";
#else
    std::string cmd = shellQuote(bin) + " --headless --disable-gpu --window-size=" +
                      std::to_string(width) + "," + std::to_string(height) +
                      " --screenshot=" + shellQuote(out) + " " + shellQuote(url) + " >/dev/null 2>&1";
#endif
    return Value(runCmd(cmd) == 0);
}

// browserDumpDom(url, outputPath)
static Value browserDumpDom(Interpreter*, std::vector<Value> args) {
    if (args.size() < 2) return Value(false);
    std::string url = asString(args[0]);
    std::string out = asString(args[1]);
    if (url.empty() || out.empty()) return Value(false);

    std::string bin = findBrowserBinary();
    if (bin.empty()) return Value(false);

#if defined(_WIN32) || defined(_WIN64)
    std::string cmd = shellQuote(bin) + " --headless --disable-gpu --dump-dom " +
                      shellQuote(url) + " > " + shellQuote(out) + " 2>NUL";
#else
    std::string cmd = shellQuote(bin) + " --headless --disable-gpu --dump-dom " +
                      shellQuote(url) + " > " + shellQuote(out) + " 2>/dev/null";
#endif
    return Value(runCmd(cmd) == 0);
}

// browserPdf(url, outputPath)
static Value browserPdf(Interpreter*, std::vector<Value> args) {
    if (args.size() < 2) return Value(false);
    std::string url = asString(args[0]);
    std::string out = asString(args[1]);
    if (url.empty() || out.empty()) return Value(false);

    std::string bin = findBrowserBinary();
    if (bin.empty()) return Value(false);

#if defined(_WIN32) || defined(_WIN64)
    std::string cmd = shellQuote(bin) + " --headless --disable-gpu --print-to-pdf=" +
                      shellQuote(out) + " " + shellQuote(url) + " >NUL 2>&1";
#else
    std::string cmd = shellQuote(bin) + " --headless --disable-gpu --print-to-pdf=" +
                      shellQuote(out) + " " + shellQuote(url) + " >/dev/null 2>&1";
#endif
    return Value(runCmd(cmd) == 0);
}

extern "C" void melt_register(Interpreter* interp) {
    interp->registerBuiltin("headlessBrowserVersion", headlessBrowserVersion);
    interp->registerBuiltin("browserAvailable", browserAvailable);
    interp->registerBuiltin("browserScreenshot", browserScreenshot);
    interp->registerBuiltin("browserDumpDom", browserDumpDom);
    interp->registerBuiltin("browserPdf", browserPdf);
}
