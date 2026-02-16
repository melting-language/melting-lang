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
            std::cout << "Melt - A simple programming language\n";
            std::cout << "Usage: melt <file.melt>\n\n";
            source = R"(
let x = 10;
let name = "Melt";
print "Hello from ";
print name;
print "x = ";
print x;
print "x + 5 = ";
print x + 5;
if (x > 5) {
    print "x is greater than 5";
} else {
    print "x is 5 or less";
}
let i = 0;
while (i < 3) {
    print i;
    i = i + 1;
}
)";
            std::cout << "Running built-in example:\n";
        }

        Lexer lexer(source);
        auto tokens = lexer.tokenize();

        Parser parser(std::move(tokens));
        auto statements = parser.parse();

        Interpreter interpreter;
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
