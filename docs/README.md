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

## Quick reference

- **Run a script:** `./bin/melt script.melt` (from project root)
- **Build with MySQL:** `make with-mysql`
- **Install binary:** `make install` (optional; installs to `/usr/local/bin` by default)
- **File extension:** `.melt`
