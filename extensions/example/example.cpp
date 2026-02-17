// Example loadable extension for Melt. Build as shared library and place in bin/modules/.
// Enable with extension = example in melt.ini.

#include "interpreter.hpp"
#include <cmath>

static Value exampleVersion(Interpreter*, std::vector<Value> args) {
    (void)args;
    return Value(std::string("1.0"));
}

static Value exampleAdd(Interpreter*, std::vector<Value> args) {
    if (args.size() < 2) return Value(0.0);
    double a = 0, b = 0;
    if (auto* p = std::get_if<double>(&args[0])) a = *p;
    if (auto* p = std::get_if<double>(&args[1])) b = *p;
    return Value(a + b);
}

static Value exampleSqrt(Interpreter*, std::vector<Value> args) {
    if (args.empty()) return Value(0.0);
    double x = 0;
    if (auto* p = std::get_if<double>(&args[0])) x = *p;
    return Value(std::sqrt(x < 0 ? 0 : x));
}

extern "C" void melt_register(Interpreter* interp) {
    interp->registerBuiltin("exampleVersion", exampleVersion);
    interp->registerBuiltin("exampleAdd", exampleAdd);
    interp->registerBuiltin("exampleSqrt", exampleSqrt);
}
