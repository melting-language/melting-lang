// Image optimization extension for Melt.
// Build as shared library and place in bin/modules/.
// Enable with: extension = image_optimize in melt.ini or melt.config.

#include "interpreter.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>

static std::string asString(const Value& v) {
    if (auto* s = std::get_if<std::string>(&v)) return *s;
    if (auto* d = std::get_if<double>(&v)) return std::to_string((int)*d);
    if (auto* b = std::get_if<bool>(&v)) return *b ? "true" : "false";
    return "";
}

static double asNumber(const Value& v, double def = 0.0) {
    if (auto* d = std::get_if<double>(&v)) return *d;
    return def;
}

static std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

static std::string extOf(const std::string& path) {
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) return "";
    return lower(path.substr(dot + 1));
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

static int runCmd(const std::string& cmd) {
    return std::system(cmd.c_str());
}

// optimizeImage(inputPath, outputPath, quality?)
static Value optimizeImage(Interpreter*, std::vector<Value> args) {
    if (args.size() < 2) return Value(false);
    std::string in = asString(args[0]);
    std::string out = asString(args[1]);
    int quality = (int)asNumber(args.size() > 2 ? args[2] : Value(80.0), 80.0);
    if (quality < 1) quality = 1;
    if (quality > 100) quality = 100;
    if (in.empty() || out.empty()) return Value(false);

    std::string ext = extOf(out.empty() ? in : out);
    int rc = 1;

#if defined(__APPLE__)
    // macOS built-in image tool.
    if (ext == "jpg" || ext == "jpeg") {
        std::string cmd = "sips -s format jpeg -s formatOptions " + std::to_string(quality) +
                          " " + shellQuote(in) + " --out " + shellQuote(out) + " >/dev/null 2>&1";
        rc = runCmd(cmd);
    } else if (ext == "png") {
        // Try pngquant for PNG if available; fallback to sips copy.
        std::string pq = "pngquant --quality " + std::to_string(std::max(1, quality - 20)) + "-" +
                         std::to_string(quality) + " --output " + shellQuote(out) +
                         " --force -- " + shellQuote(in) + " >/dev/null 2>&1";
        rc = runCmd(pq);
        if (rc != 0) {
            std::string cmd = "sips -s format png " + shellQuote(in) +
                              " --out " + shellQuote(out) + " >/dev/null 2>&1";
            rc = runCmd(cmd);
        }
    } else {
        std::string cmd = "sips " + shellQuote(in) + " --out " + shellQuote(out) + " >/dev/null 2>&1";
        rc = runCmd(cmd);
    }
#elif defined(_WIN32) || defined(_WIN64)
    // Windows fallback: require ImageMagick convert/magick if installed.
    std::string cmd = "magick " + shellQuote(in) + " -quality " + std::to_string(quality) +
                      " " + shellQuote(out) + " >NUL 2>&1";
    rc = runCmd(cmd);
#else
    // Linux fallback: require ImageMagick convert/magick if installed.
    std::string cmd = "magick " + shellQuote(in) + " -quality " + std::to_string(quality) +
                      " " + shellQuote(out) + " >/dev/null 2>&1";
    rc = runCmd(cmd);
    if (rc != 0) {
        cmd = "convert " + shellQuote(in) + " -quality " + std::to_string(quality) +
              " " + shellQuote(out) + " >/dev/null 2>&1";
        rc = runCmd(cmd);
    }
#endif

    return Value(rc == 0);
}

// resizeImage(inputPath, outputPath, width, height, quality?)
static Value resizeImage(Interpreter*, std::vector<Value> args) {
    if (args.size() < 4) return Value(false);
    std::string in = asString(args[0]);
    std::string out = asString(args[1]);
    int w = (int)asNumber(args[2], 0.0);
    int h = (int)asNumber(args[3], 0.0);
    int quality = (int)asNumber(args.size() > 4 ? args[4] : Value(80.0), 80.0);
    if (w <= 0 || h <= 0 || in.empty() || out.empty()) return Value(false);
    if (quality < 1) quality = 1;
    if (quality > 100) quality = 100;

    int rc = 1;
#if defined(__APPLE__)
    std::string cmd = "sips -z " + std::to_string(h) + " " + std::to_string(w) +
                      " " + shellQuote(in) + " --out " + shellQuote(out) + " >/dev/null 2>&1";
    rc = runCmd(cmd);
    if (rc == 0) {
        std::string ext = extOf(out);
        if (ext == "jpg" || ext == "jpeg") {
            cmd = "sips -s format jpeg -s formatOptions " + std::to_string(quality) +
                  " " + shellQuote(out) + " >/dev/null 2>&1";
            rc = runCmd(cmd);
        }
    }
#elif defined(_WIN32) || defined(_WIN64)
    std::string cmd = "magick " + shellQuote(in) + " -resize " + std::to_string(w) + "x" + std::to_string(h) +
                      " -quality " + std::to_string(quality) + " " + shellQuote(out) + " >NUL 2>&1";
    rc = runCmd(cmd);
#else
    std::string cmd = "magick " + shellQuote(in) + " -resize " + std::to_string(w) + "x" + std::to_string(h) +
                      " -quality " + std::to_string(quality) + " " + shellQuote(out) + " >/dev/null 2>&1";
    rc = runCmd(cmd);
    if (rc != 0) {
        cmd = "convert " + shellQuote(in) + " -resize " + std::to_string(w) + "x" + std::to_string(h) +
              " -quality " + std::to_string(quality) + " " + shellQuote(out) + " >/dev/null 2>&1";
        rc = runCmd(cmd);
    }
#endif

    return Value(rc == 0);
}

static Value imageOptimizeVersion(Interpreter*, std::vector<Value>) {
    return Value(std::string("1.0"));
}

extern "C" void melt_register(Interpreter* interp) {
    interp->registerBuiltin("imageOptimizeVersion", imageOptimizeVersion);
    interp->registerBuiltin("optimizeImage", optimizeImage);
    interp->registerBuiltin("resizeImage", resizeImage);
}
