# Melt - A Simple Object-Oriented Language

A minimal interpreted, object-oriented programming language built in C++.

**Full documentation:** [docs/README.md](docs/README.md) — getting started, language reference, built-ins, HTTP server & MVC framework, examples. **HTML version:** [docs/html/index.html](docs/html/index.html) (open in a browser).

## Features

- **Variables**: `let x = 10;`
- **Strings**: `let name = "hello";`
- **Arithmetic**: `+`, `-`, `*`, `/`
- **Comparisons**: `==`, `!=`, `<`, `<=`, `>`, `>=`
- **Comments**: `//` line comment; `/* ... */` block comment (ignored by the lexer).
- **Control flow**: `if` / `else`, `while`
- **Print**: `print expr;`
- **Classes**: `class Name { method init(...) { ... } method name(...) { ... } }`
- **Objects**: `let obj = ClassName(args);` — constructor runs `init` with arguments
- **Methods**: `obj.method(args);` — `this` refers to the object. Use `return expr;` or `return;` to return a value (or nothing); the call expression yields that value.
- **Properties**: `this.x = value;`, `obj.field`, `obj.field = value;`
- **Import**: `import "path.melt";` — run another file (path relative to current file); classes and variables are shared.
- **HTTP server (backend)**: built-ins `getRequestPath()`, `getRequestMethod()`, `getRequestBody()`, `setResponseBody(str)`, `setResponseStatus(code)`, `setResponseContentType(str)`, `servePublic(path)` (serve files from `public/js`, `public/css`, `public/images`), `setHandler("ClassName")`, `listen(port)`. Define a class with `method handle()` and use it as the request handler.
- **MySQL (optional)**: build with `make with-mysql`. Built-ins: `mysqlConnect(host, user, password, database)` → true/false, `mysqlQuery(sql)` → true/false, `mysqlFetchRow()` → next row as tab-separated string (or "" when no more), `mysqlClose()`.
- **File I/O**: `readFile(path)` → file contents as string (or "" on error), `writeFile(path, content)` → true/false.
- **Arrays**: Literal `[1, 2, 3]`, index `arr[i]`, assignment `arr[i] = value`, and built-ins `arrayCreate()`, `arrayPush(arr, value)`, `arrayGet(arr, i)`, `arraySet(arr, i, value)`, `arrayLength(arr)`.
- **JSON**: `jsonEncode(value)` → JSON string; `jsonDecode(str)` → value (arrays and objects). Decoded objects support `obj.field` access.
- **Encoding / simple crypto**: `base64Encode(str)`, `base64Decode(str)`; `xorCipher(str, key)` for symmetric XOR (same key encrypts and decrypts; weak, for obfuscation only).
- **Blade-like views**: `renderView(templatePath, data)` loads an HTML template (path relative to script dir) and replaces `{{ key }}` (escaped) and `{!! key !!}` (raw) with fields from the `data` object. Used in the `web_project_mvc` example.

## Build

```bash
make
```

This creates the **`bin/melt`** executable (the `bin/` directory is created if needed).

For a smaller binary without HTTP server, MySQL, or GUI (e.g. for embedded or resource-constrained targets), use **`make embedded`** → **`bin/melt-embedded`**. Scripts that call `listen()`, MySQL built-ins, or `imagePreview()` will get a clear runtime error.

## Install (binary)

Install the `melt` binary so you can run it from anywhere:

```bash
make install
```

This installs to `/usr/local/bin` by default. Use a different prefix if needed:

```bash
make install PREFIX=/usr          # system-wide (e.g. Linux)
make install PREFIX=$(HOME)/.local # user-only (add ~/.local/bin to PATH)
```

To remove: `make uninstall` (or `make uninstall PREFIX=...` if you changed it).

## Run

The compiled binary is `bin/melt`. From the project root:

```bash
./bin/melt example.melt
```

Or run without arguments to execute a built-in demo:

```bash
./bin/melt
```

## Example

```melt
let x = 10;
let name = "Melt";

print "Hello from ";
print name;
print x + 5;

if (x > 5) {
    print "x is greater than 5";
}

let i = 0;
while (i < 3) {
    print i;
    i = i + 1;
}
```

## Syntax

- Statements end with `;`
- Blocks use `{` and `}`
- Conditions: `if (expr) stmt` and `if (expr) stmt else stmt`
- Loops: `while (expr) stmt`
- Classes: `class Name { method name(params) { body } }`
- Constructor: `init(params)` is run when you call `ClassName(args)`
- Methods take parameters: `method add(other) { this.x = this.x + other.x; }`
- Imports: `import "other.melt";` — path is relative to the importing file’s directory.

## Backend HTTP server

```bash
./bin/melt examples/server.melt
```

Then open http://localhost:8080/ in a browser. The handler class must define `method handle()` and use the built-ins above to read the request and set the response.

## MySQL (optional)

Install the MySQL client library (e.g. `libmysqlclient-dev` on Ubuntu, `mysql` on Homebrew), then:

```bash
make with-mysql
./bin/melt examples/mysql_example.melt
```

Edit `examples/mysql_example.melt` and set host, user, password, and database. Use `mysqlConnect(host, user, password, db)`, then `mysqlQuery("SELECT ...")`, then loop with `mysqlFetchRow()` to get each row as a tab-separated string. Call `mysqlClose()` when done.

## Multi-file / import example

```bash
./bin/melt examples/multi_file/main.melt
```

`examples/multi_file/` has `point.melt`, `greeter.melt`, and `main.melt` that imports them.

## OOP Example

```bash
./bin/melt example_oop.melt
```

See `example_oop.melt` for a full example with `Point` and `Greeter` classes.

---

## Documentation

| Document | Description |
|----------|-------------|
| [docs/README.md](docs/README.md) | Documentation index |
| [Getting Started](docs/01_GETTING_STARTED.md) | Build, install, run |
| [Language Reference](docs/02_LANGUAGE_REFERENCE.md) | Syntax, types, classes, methods, return, import |
| [Built-ins](docs/03_BUILTINS.md) | All built-in functions |
| [HTTP & MVC](docs/04_HTTP_AND_MVC.md) | Server and web_project_mvc |
| [Examples](docs/05_EXAMPLES.md) | Example programs index |