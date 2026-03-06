#include "module_loader.hpp"
#include "interpreter.hpp"
#include <iostream>
#include <string>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace {

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end == std::string::npos ? std::string::npos : end - start + 1);
}

static std::vector<std::string> splitComma(const std::string& list) {
    std::vector<std::string> out;
    size_t pos = 0;
    while (pos < list.size()) {
        size_t next = list.find(',', pos);
        std::string part = trim(next == std::string::npos ? list.substr(pos) : list.substr(pos, next - pos));
        if (!part.empty()) out.push_back(part);
        if (next == std::string::npos) break;
        pos = next + 1;
    }
    return out;
}

static std::string extensionSuffix() {
#if defined(_WIN32) || defined(_WIN64)
    return ".dll";
#elif defined(__APPLE__)
    return ".dylib";
#else
    return ".so";
#endif
}

using MeltRegisterFn = void (*)(Interpreter*);

#if defined(_WIN32) || defined(_WIN64)
static void loadOne(Interpreter* interp, const std::string& path) {
    std::wstring wpath(path.begin(), path.end());
    HMODULE h = LoadLibraryW(wpath.c_str());
    if (!h) {
        DWORD err = GetLastError();
        (void)err;
        std::cerr << "melt: could not load extension '" << path << "' (LoadLibrary error " << err << ")\n";
        return;
    }
    auto fn = (MeltRegisterFn)GetProcAddress(h, "melt_register");
    if (!fn) {
        std::cerr << "melt: extension '" << path << "' has no melt_register symbol\n";
        FreeLibrary(h);
        return;
    }
    fn(interp);
    (void)interp;
}
#else
static void loadOne(Interpreter* interp, const std::string& path) {
    void* h = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
#if defined(__APPLE__)
    if (!h && path.size() >= 6 && path.compare(path.size() - 6, 6, ".dylib") == 0) {
        std::string alt = path.substr(0, path.size() - 6) + ".so";
        h = dlopen(alt.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!h) {
            const char* err = dlerror();
            std::cerr << "melt: could not load extension '" << path << "': " << (err ? err : "unknown") << "\n";
            return;
        }
    } else
#endif
    if (!h) {
        const char* err = dlerror();
        std::cerr << "melt: could not load extension '" << path << "': " << (err ? err : "unknown") << "\n";
        return;
    }
    void* sym = dlsym(h, "melt_register");
    if (!sym) {
        std::cerr << "melt: extension '" << path << "' has no melt_register symbol\n";
        dlclose(h);
        return;
    }
    MeltRegisterFn fn = reinterpret_cast<MeltRegisterFn>(sym);
    fn(interp);
}
#endif

} // namespace

void loadExtensions(Interpreter* interp, const std::string& binDir,
                   const std::string& extensionDir, const std::string& extensionList) {
    if (binDir.empty() || extensionList.empty()) return;
    std::string base = extensionDir.empty() ? binDir : (binDir + "/" + extensionDir);
    std::string suffix = extensionSuffix();
    for (const std::string& name : splitComma(extensionList)) {
        std::string path = base + "/" + name + suffix;
        loadOne(interp, path);
    }
}
