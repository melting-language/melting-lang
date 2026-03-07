# Melt Language Documentation

Documentation for the **Melt** programming language: a minimal, interpreted, object-oriented language implemented in C++.

**HTML version (single page, with navigation):** [html/index.html](html/index.html) — open in a browser for the full documentation with an attractive, professional layout.

## Contents

| Document | Description |
|----------|-------------|
| [Getting Started](01_GETTING_STARTED.md) | Build, install, run your first program |
| [Language Reference](02_LANGUAGE_REFERENCE.md) | Syntax, types, expressions, statements, control flow, classes, methods, return, import |
| [Built-in Functions](03_BUILTINS.md) | All built-ins: I/O, arrays, JSON, HTTP, MySQL, encryption, views |
| [HTTP Server & MVC Framework](04_HTTP_AND_MVC.md) | Backend server, web_project_mvc and official website (official_website_using_melt): config, routes, controllers, views, blog, migrations |
| [Examples](05_EXAMPLES.md) | Index of example programs and how to run them |
| [Tutorial: Implement syntax](tutorial_implement_syntax.md) | Hands-on tutorial: add a `say expr;` statement (lexer → AST → parser → interpreter) |
| [Tutorial (HTML)](tutorial_implement_syntax.html) | Same tutorial in HTML with sidebar (open in browser) |
| [Development: Adding syntax](06_DEVELOPMENT_ADDING_SYNTAX.md) | Reference guide to implement new syntax (checklist, expressions, blocks) |

## Quick reference

- **Run a script:** `./build/melt script.melt` (from project root)
- **Build with MySQL:** `cmake -B build -DUSE_MYSQL=ON && cmake --build build`
- **Install binary:** `cmake --install build` (optional; installs to `/usr/local` by default)
- **File extension:** `.melt`
