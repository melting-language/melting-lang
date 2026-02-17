#pragma once

#include <string>

class Interpreter;

// Load extension shared libraries from binDir/extensionDir/ and call melt_register(interp) for each
// extension in extensionList (comma-separated names). On failure to open or find symbol, log to stderr and continue.
void loadExtensions(Interpreter* interp, const std::string& binDir,
                   const std::string& extensionDir, const std::string& extensionList);
