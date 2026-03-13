#include "lexer.hpp"
#include "parser.hpp"
#include "interpreter.hpp"
#include "config.h"
#ifndef PROJECT_VERSION_MAJOR
#define PROJECT_VERSION_MAJOR 1
#define PROJECT_VERSION_MINOR 3
#define PROJECT_VERSION_PATCH 2
#endif
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <filesystem>
#include <string>

std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Cannot open file: " + path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static void printHelp() {
    std::cout << "Melt - A lightweight object-oriented programming language\n\n";
    std::cout << "Usage:\n";
    std::cout << "  melt [options] <file.melt>    Run a Melt script.\n";
    std::cout << "  melt -e <code>                 Run inline code.\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help                     Show this help and exit.\n";
    std::cout << "  -v, --version                  Show version and exit.\n";
    std::cout << "  -c, --check                    Parse only (syntax check), do not run.\n";
    std::cout << "  -e <code>                      Execute <code> as Melt script (no file).\n";
    std::cout << "  --trace                        Trace executed statements to stderr.\n";
    std::cout << "  --recursion-limit <N>          Max call depth (0 = no limit). Default 0.\n\n";
    std::cout << "Examples:\n";
    std::cout << "  melt example.melt\n";
    std::cout << "  melt -c script.melt\n";
    std::cout << "  melt -e 'print 1 + 2;'\n";
    std::cout << "  melt --trace examples/server.melt\n\n";
    std::cout << "Help & documentation:\n";
    std::cout << "  https://github.com/melting-language/melting-lang\n";
    std::cout << "  docs/README.md in the project root\n\n";
    std::cout << "Support:\n";
    std::cout << "  GitHub Discussions / Issues: https://github.com/melting-language/melting-lang/issues\n";
}

static void printVersion() {
    std::cout << "melt " << PROJECT_VERSION_MAJOR << "." << PROJECT_VERSION_MINOR << "." << PROJECT_VERSION_PATCH << "\n";
}

int main(int argc, char* argv[]) {
    try {
        std::string source;
        std::string entryPath;
        std::string sourcePath;
        bool checkOnly = false;
        bool trace = false;
        int recursionLimit = 0;
        int scriptArgIndex = -1;
        bool useInline = false;

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i] ? argv[i] : "";
            if (arg == "-h" || arg == "--help") {
                printHelp();
                return 0;
            }
            if (arg == "-v" || arg == "--version") {
                printVersion();
                return 0;
            }
            if (arg == "-c" || arg == "--check") {
                checkOnly = true;
                continue;
            }
            if (arg == "-e") {
                if (i + 1 >= argc) {
                    std::cerr << "Error: -e requires inline code argument.\n";
                    return 1;
                }
                source = argv[++i];
                useInline = true;
                entryPath = "";
                sourcePath = "<inline>";
                continue;
            }
            if (arg == "--trace") {
                trace = true;
                continue;
            }
            if (arg == "--recursion-limit") {
                if (i + 1 >= argc) {
                    std::cerr << "Error: --recursion-limit requires a number.\n";
                    return 1;
                }
                try {
                    recursionLimit = std::stoi(argv[++i]);
                    if (recursionLimit < 0) recursionLimit = 0;
                } catch (...) {
                    std::cerr << "Error: --recursion-limit must be a non-negative number.\n";
                    return 1;
                }
                continue;
            }
            if (arg.empty() || (arg[0] == '-' && arg.size() > 1 && arg[1] != '-')) {
                /* treat as positional script path only if we didn't use -e */
                if (!useInline && scriptArgIndex < 0) scriptArgIndex = i;
                break;
            }
            if (arg.compare(0, 2, "--") == 0) {
                /* unknown long option; treat as script path */
                if (!useInline && scriptArgIndex < 0) scriptArgIndex = i;
                break;
            }
            if (!useInline && scriptArgIndex < 0) scriptArgIndex = i;
            break;
        }

        if (!useInline && scriptArgIndex < 0) {
            printHelp();
            return 0;
        }

        if (!useInline) {
            std::string path = argv[scriptArgIndex];
            source = readFile(path);
            sourcePath = path;
            try {
                entryPath = std::filesystem::absolute(path).generic_string();
            } catch (...) {
                entryPath = path;
            }
        }

        Lexer lexer(source);
        auto tokens = lexer.tokenize();

        Parser parser(std::move(tokens), sourcePath);
        auto statements = parser.parse();

        if (checkOnly) {
            std::cout << "Syntax OK\n";
            return 0;
        }

        std::string binDir;
        if (argc >= 1 && argv[0] && argv[0][0]) {
            try {
                std::string exePath = argv[0];
                if (!exePath.empty() && exePath[0] != '/' && (exePath.size() < 2 || exePath[1] != ':'))
                    exePath = std::filesystem::current_path().generic_string() + "/" + exePath;
                std::filesystem::path p = std::filesystem::canonical(exePath);
                binDir = p.parent_path().generic_string();
            } catch (...) { /* leave binDir empty */ }
        }

        Interpreter interpreter;
        interpreter.setBinDir(binDir);
        interpreter.setTrace(trace);
        interpreter.setRecursionLimit(recursionLimit);
        interpreter.interpret(statements, entryPath);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
