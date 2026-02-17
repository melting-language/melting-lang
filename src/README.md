# Melt source layout

| Directory | Contents |
|-----------|----------|
| **core/** | Language core: lexer, parser, AST, interpreter |
| **http/** | HTTP server (built-in `listen`, `setHandler`, etc.) |
| **mysql/** | MySQL builtin (optional, `make with-mysql`) |
| **gui/** | Image preview window (optional, `make with-gui`) |
| *root* | `main.cpp` — entry point, CLI, file loading |

Include path: the Makefile adds `-I src/core -I src/http -I src/mysql -I src/gui`, so `#include "lexer.hpp"` and similar resolve from the correct subdirectory.
