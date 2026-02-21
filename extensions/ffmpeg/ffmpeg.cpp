// FFmpeg extension for Melt.
// Build as shared library and place in bin/modules/.
// Enable with: extension = ffmpeg in melt.ini or melt.config.
// Requires ffmpeg (and ffprobe for probe functions) on PATH.

#include "interpreter.hpp"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

static std::string asString(const Value& v) {
    if (auto* s = std::get_if<std::string>(&v)) return *s;
    if (auto* d = std::get_if<double>(&v)) return std::to_string((int)*d);
    if (auto* b = std::get_if<bool>(&v)) return *b ? "true" : "false";
    return "";
}

static int runCmd(const std::string& cmd) {
    return std::system(cmd.c_str());
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

static std::string runAndCapture(const std::string& cmd) {
#if defined(_WIN32) || defined(_WIN64)
    std::string tmp = std::string(std::getenv("TEMP") ? std::getenv("TEMP") : ".") + "\\melt_ffprobe.tmp";
    std::string full = cmd + " > " + shellQuote(tmp) + " 2>NUL";
    if (runCmd(full) != 0) return "";
    std::ifstream f(tmp);
    std::stringstream buf;
    if (f) buf << f.rdbuf();
    remove(tmp.c_str());
    return buf.str();
#else
    FILE* fp = popen((cmd + " 2>/dev/null").c_str(), "r");
    if (!fp) return "";
    std::string out;
    char buf[256];
    while (fgets(buf, sizeof(buf), fp)) out += buf;
    pclose(fp);
    return out;
#endif
}

static Value ffmpegVersion(Interpreter*, std::vector<Value>) {
    return Value(std::string("1.0"));
}

static Value ffmpegAvailable(Interpreter*, std::vector<Value>) {
    return Value(commandExists("ffmpeg"));
}

static Value ffprobeAvailable(Interpreter*, std::vector<Value>) {
    return Value(commandExists("ffprobe"));
}

// ffmpegConvert(inputPath, outputPath [, extraOptions]) — transcode; extraOptions optional (e.g. "-b:v 1M")
static Value ffmpegConvert(Interpreter*, std::vector<Value> args) {
    if (args.size() < 2) return Value(false);
    std::string in = asString(args[0]);
    std::string out = asString(args[1]);
    std::string extra = args.size() > 2 ? asString(args[2]) : "";
    if (in.empty() || out.empty()) return Value(false);

#if defined(_WIN32) || defined(_WIN64)
    std::string cmd = "ffmpeg -y -i " + shellQuote(in);
    if (!extra.empty()) cmd += " " + extra;
    cmd += " " + shellQuote(out) + " >NUL 2>&1";
#else
    std::string cmd = "ffmpeg -y -i " + shellQuote(in);
    if (!extra.empty()) cmd += " " + extra;
    cmd += " " + shellQuote(out) + " >/dev/null 2>&1";
#endif
    return Value(runCmd(cmd) == 0);
}

// ffmpegExtractAudio(inputPath, outputPath [, codec]) — codec: "mp3" (default), "aac", "opus"
static Value ffmpegExtractAudio(Interpreter*, std::vector<Value> args) {
    if (args.size() < 2) return Value(false);
    std::string in = asString(args[0]);
    std::string out = asString(args[1]);
    std::string codec = args.size() > 2 ? asString(args[2]) : "mp3";
    if (in.empty() || out.empty()) return Value(false);

    std::string acodec;
    if (codec == "mp3") acodec = "libmp3lame";
    else if (codec == "aac") acodec = "aac";
    else if (codec == "opus") acodec = "libopus";
    else acodec = "libmp3lame";

#if defined(_WIN32) || defined(_WIN64)
    std::string cmd = "ffmpeg -y -i " + shellQuote(in) + " -vn -acodec " + acodec +
                      " " + shellQuote(out) + " >NUL 2>&1";
#else
    std::string cmd = "ffmpeg -y -i " + shellQuote(in) + " -vn -acodec " + acodec +
                      " " + shellQuote(out) + " >/dev/null 2>&1";
#endif
    return Value(runCmd(cmd) == 0);
}

// ffmpegToGif(inputPath, outputPath [, width]) — video to animated GIF; width optional (e.g. 320)
static Value ffmpegToGif(Interpreter*, std::vector<Value> args) {
    if (args.size() < 2) return Value(false);
    std::string in = asString(args[0]);
    std::string out = asString(args[1]);
    int width = 320;
    if (args.size() > 2 && std::holds_alternative<double>(args[2]))
        width = (int)std::get<double>(args[2]);
    if (in.empty() || out.empty() || width <= 0) return Value(false);

#if defined(_WIN32) || defined(_WIN64)
    std::string cmd = "ffmpeg -y -i " + shellQuote(in) +
                      " -vf \"scale=" + std::to_string(width) + ":-1:flags=lanczos,split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse\" "
                      "-loop 0 " + shellQuote(out) + " >NUL 2>&1";
#else
    std::string cmd = "ffmpeg -y -i " + shellQuote(in) +
                      " -vf \"scale=" + std::to_string(width) + ":-1:flags=lanczos,split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse\" "
                      "-loop 0 " + shellQuote(out) + " >/dev/null 2>&1";
#endif
    return Value(runCmd(cmd) == 0);
}

// ffprobeDuration(inputPath) — duration in seconds, or -1 on error
static Value ffprobeDuration(Interpreter*, std::vector<Value> args) {
    if (args.empty()) return Value(-1.0);
    std::string in = asString(args[0]);
    if (in.empty()) return Value(-1.0);

    std::string cmd = "ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 " + shellQuote(in);
    std::string out = runAndCapture(cmd);
    while (!out.empty() && (out.back() == '\r' || out.back() == '\n')) out.pop_back();
    if (out.empty()) return Value(-1.0);
    try {
        return Value(std::stod(out));
    } catch (...) {
        return Value(-1.0);
    }
}

// ffprobeInfo(inputPath) — JSON string with format and streams (use jsonDecode in Melt)
static Value ffprobeInfo(Interpreter*, std::vector<Value> args) {
    if (args.empty()) return Value(std::string(""));
    std::string in = asString(args[0]);
    if (in.empty()) return Value(std::string(""));

    std::string cmd = "ffprobe -v quiet -print_format json -show_format -show_streams " + shellQuote(in);
    std::string out = runAndCapture(cmd);
    return Value(out);
}

static double asNumber(const Value& v, double def = 0.0) {
    if (auto* d = std::get_if<double>(&v)) return *d;
    return def;
}

// ffmpegGenerateTestVideo(outputPath [, duration [, width [, height]]]) — generate short test video (testsrc)
static Value ffmpegGenerateTestVideo(Interpreter*, std::vector<Value> args) {
    if (args.empty() || !std::holds_alternative<std::string>(args[0])) return Value(false);
    std::string out = std::get<std::string>(args[0]);
    if (out.empty()) return Value(false);
    double duration = 2.0;
    int width = 320;
    int height = 240;
    if (args.size() > 1) duration = asNumber(args[1], 2.0);
    if (args.size() > 2) width = (int)asNumber(args[2], 320);
    if (args.size() > 3) height = (int)asNumber(args[3], 240);
    if (duration <= 0 || duration > 60) duration = 2.0;
    if (width <= 0) width = 320;
    if (height <= 0) height = 240;

#if defined(_WIN32) || defined(_WIN64)
    std::string cmd = "ffmpeg -y -f lavfi -i testsrc=duration=" + std::to_string((int)duration) +
                      ":size=" + std::to_string(width) + "x" + std::to_string(height) + ":rate=10 "
                      "-pix_fmt yuv420p " + shellQuote(out) + " >NUL 2>&1";
#else
    std::string cmd = "ffmpeg -y -f lavfi -i testsrc=duration=" + std::to_string((int)duration) +
                      ":size=" + std::to_string(width) + "x" + std::to_string(height) + ":rate=10 "
                      "-pix_fmt yuv420p " + shellQuote(out) + " >/dev/null 2>&1";
#endif
    return Value(runCmd(cmd) == 0);
}

extern "C" void melt_register(Interpreter* interp) {
    interp->registerBuiltin("ffmpegVersion", ffmpegVersion);
    interp->registerBuiltin("ffmpegAvailable", ffmpegAvailable);
    interp->registerBuiltin("ffprobeAvailable", ffprobeAvailable);
    interp->registerBuiltin("ffmpegConvert", ffmpegConvert);
    interp->registerBuiltin("ffmpegExtractAudio", ffmpegExtractAudio);
    interp->registerBuiltin("ffmpegToGif", ffmpegToGif);
    interp->registerBuiltin("ffprobeDuration", ffprobeDuration);
    interp->registerBuiltin("ffprobeInfo", ffprobeInfo);
    interp->registerBuiltin("ffmpegGenerateTestVideo", ffmpegGenerateTestVideo);
}
