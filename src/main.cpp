#include "lexer.hpp"
#include "parser.hpp"
#include "interpreter.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <filesystem>

std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Cannot open file: " + path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

int main(int argc, char* argv[]) {
    try {
        std::string source;
        if (argc > 1) {
            source = readFile(argv[1]);
        } else {
            std::cout << "Melt - A lighweight object-oriented programming language\n\n";
            std::cout << "Usage:\n";
            std::cout << "  melt <file.melt>    Run a Melt script.\n\n";
            std::cout << "Examples:\n";
            std::cout << "  melt example.melt\n";
            std::cout << "  melt examples/server.melt\n\n";
            std::cout << "Help & documentation:\n";
            std::cout << "  https://github.com/melting-language/melting-lang\n";
            std::cout << "  docs/README.md in the project root\n\n";
            std::cout << "Support:\n";
            std::cout << "  GitHub Discussions / Issues: https://github.com/melting-language/melting-lang/issues\n";
            return 0;
        }

        Lexer lexer(source);
        auto tokens = lexer.tokenize();

        Parser parser(std::move(tokens));
        auto statements = parser.parse();

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
        std::string entryPath = "";
        if (argc > 1) {
            try {
                entryPath = std::filesystem::absolute(argv[1]).generic_string();
            } catch (...) {
                entryPath = argv[1];
            }
        }
        interpreter.interpret(statements, entryPath);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
