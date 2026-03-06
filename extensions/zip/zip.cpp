// Zip / compression extension for Melt.
// Requires libzip. Enable in melt.ini: extension = zip
// Functions: zipVersion, zipCreate, zipExtract, zipList, zipReadEntry, zipAddFiles.

#include "interpreter.hpp"
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullability-extension"
#endif
#include <zip.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#include <cerrno>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#include <sys/stat.h>
#define mkdir(path, mode) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

static std::string asString(const Value& v) {
    if (auto* s = std::get_if<std::string>(&v)) return *s;
    if (auto* d = std::get_if<double>(&v)) return std::to_string(static_cast<long long>(*d));
    if (auto* b = std::get_if<bool>(&v)) return *b ? "true" : "false";
    return "";
}

static std::string basename(const std::string& path) {
    size_t i = path.find_last_of("/\\");
    if (i == std::string::npos) return path;
    return path.substr(i + 1);
}

static bool ensureDir(const std::string& dir) {
    if (dir.empty()) return true;
    std::string p;
    for (size_t i = 0; i <= dir.size(); ++i) {
        if (i < dir.size() && dir[i] != '/' && dir[i] != '\\') continue;
        p = dir.substr(0, i);
        if (p.empty() || p == "." || p == "..") continue;
#ifdef _WIN32
        if (mkdir(p.c_str()) != 0 && errno != EEXIST) return false;
#else
        if (mkdir(p.c_str(), 0755) != 0 && errno != EEXIST) return false;
#endif
    }
    return true;
}

static Value zipVersion(Interpreter*, std::vector<Value>) {
    return Value(std::string("1.0"));
}

// zipCreate(zipPath, paths) — create zip at zipPath containing files/dirs from paths (array of strings)
static Value zipCreate(Interpreter*, std::vector<Value> args) {
    if (args.size() < 2) return Value(false);
    std::string zipPath = asString(args[0]);
    if (zipPath.empty()) return Value(false);
    auto* arr = std::get_if<std::shared_ptr<MeltArray>>(&args[1]);
    if (!arr || !*arr) return Value(false);

    int err = 0;
    zip_t* za = zip_open(zipPath.c_str(), ZIP_CREATE | ZIP_TRUNCATE, &err);
    if (!za) return Value(false);

    bool ok = true;
    for (size_t i = 0; i < (*arr)->data.size(); ++i) {
        std::string path = asString((*arr)->data[i]);
        if (path.empty()) continue;
        std::string name = basename(path);
        zip_source_t* src = zip_source_file(za, path.c_str(), 0, -1);
        if (!src) { ok = false; break; }
        if (zip_file_add(za, name.c_str(), src, ZIP_FL_ENC_UTF_8) < 0) {
            zip_source_free(src);
            ok = false;
            break;
        }
    }
    zip_close(za);
    return Value(ok);
}

// zipExtract(zipPath, destDir) — extract all entries to destDir
static Value zipExtract(Interpreter*, std::vector<Value> args) {
    if (args.size() < 2) return Value(false);
    std::string zipPath = asString(args[0]);
    std::string destDir = asString(args[1]);
    if (zipPath.empty()) return Value(false);
    if (destDir.empty()) destDir = ".";

    int err = 0;
    zip_t* za = zip_open(zipPath.c_str(), ZIP_RDONLY, &err);
    if (!za) return Value(false);

    zip_int64_t n = zip_get_num_entries(za, 0);
    bool ok = true;
    for (zip_int64_t i = 0; i < n && ok; ++i) {
        zip_stat_t st;
        if (zip_stat_index(za, i, 0, &st) != 0) { ok = false; break; }
        const char* name = zip_get_name(za, i, ZIP_FL_ENC_GUESS);
        if (!name) { ok = false; break; }
        std::string entryName(name);
        if (entryName.empty()) continue;
        if (entryName.back() == '/') {
            std::string dir = destDir + "/" + entryName;
            if (!ensureDir(dir)) ok = false;
            continue;
        }
        zip_file_t* zf = zip_fopen_index(za, i, 0);
        if (!zf) { ok = false; break; }
        std::string outPath = destDir + "/" + entryName;
        std::string parent = outPath.substr(0, outPath.find_last_of("/\\"));
        if (!parent.empty() && !ensureDir(parent)) { zip_fclose(zf); ok = false; break; }
        std::ofstream out(outPath, std::ios::binary);
        if (!out) { zip_fclose(zf); ok = false; break; }
        char buf[8192];
        zip_int64_t len;
        while ((len = zip_fread(zf, buf, sizeof(buf))) > 0)
            out.write(buf, len);
        zip_fclose(zf);
        if (len < 0) ok = false;
    }
    zip_close(za);
    return Value(ok);
}

// zipList(zipPath) — return array of entry names (strings)
static Value zipList(Interpreter*, std::vector<Value> args) {
    if (args.empty()) return Value(std::make_shared<MeltArray>());
    std::string zipPath = asString(args[0]);
    if (zipPath.empty()) return Value(std::make_shared<MeltArray>());

    int err = 0;
    zip_t* za = zip_open(zipPath.c_str(), ZIP_RDONLY, &err);
    if (!za) return Value(std::make_shared<MeltArray>());

    auto list = std::make_shared<MeltArray>();
    zip_int64_t n = zip_get_num_entries(za, 0);
    for (zip_int64_t j = 0; j < n; ++j) {
        const char* name = zip_get_name(za, j, ZIP_FL_ENC_GUESS);
        if (name) list->data.push_back(Value(std::string(name)));
    }
    zip_close(za);
    return Value(list);
}

// zipReadEntry(zipPath, entryName) — read one entry's contents as string (binary-safe)
static Value zipReadEntry(Interpreter*, std::vector<Value> args) {
    if (args.size() < 2) return Value(std::string(""));
    std::string zipPath = asString(args[0]);
    std::string entryName = asString(args[1]);
    if (zipPath.empty() || entryName.empty()) return Value(std::string(""));

    int err = 0;
    zip_t* za = zip_open(zipPath.c_str(), ZIP_RDONLY, &err);
    if (!za) return Value(std::string(""));

    zip_int64_t idx = zip_name_locate(za, entryName.c_str(), ZIP_FL_ENC_GUESS);
    if (idx < 0) { zip_close(za); return Value(std::string("")); }
    zip_file_t* zf = zip_fopen_index(za, idx, 0);
    if (!zf) { zip_close(za); return Value(std::string("")); }
    std::string out;
    char buf[8192];
    zip_int64_t len;
    while ((len = zip_fread(zf, buf, sizeof(buf))) > 0)
        out.append(buf, len);
    zip_fclose(zf);
    zip_close(za);
    return Value(out);
}

// zipAddFiles(zipPath, paths) — add files to existing zip (or create); paths = array of file paths
static Value zipAddFiles(Interpreter*, std::vector<Value> args) {
    if (args.size() < 2) return Value(false);
    std::string zipPath = asString(args[0]);
    if (zipPath.empty()) return Value(false);
    auto* arr = std::get_if<std::shared_ptr<MeltArray>>(&args[1]);
    if (!arr || !*arr) return Value(false);

    int err = 0;
    zip_t* za = zip_open(zipPath.c_str(), ZIP_CREATE, &err);
    if (!za) return Value(false);

    bool ok = true;
    for (size_t i = 0; i < (*arr)->data.size(); ++i) {
        std::string path = asString((*arr)->data[i]);
        if (path.empty()) continue;
        std::string name = basename(path);
        zip_source_t* src = zip_source_file(za, path.c_str(), 0, -1);
        if (!src) { ok = false; break; }
        if (zip_file_add(za, name.c_str(), src, ZIP_FL_ENC_UTF_8 | ZIP_FL_OVERWRITE) < 0) {
            zip_source_free(src);
            ok = false;
            break;
        }
    }
    zip_close(za);
    return Value(ok);
}

extern "C" void melt_register(Interpreter* interp) {
    interp->registerBuiltin("zipVersion", zipVersion);
    interp->registerBuiltin("zipCreate", zipCreate);
    interp->registerBuiltin("zipExtract", zipExtract);
    interp->registerBuiltin("zipList", zipList);
    interp->registerBuiltin("zipReadEntry", zipReadEntry);
    interp->registerBuiltin("zipAddFiles", zipAddFiles);
}
