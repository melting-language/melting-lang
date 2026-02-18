# Getting Started

## Build

From the project root:

```bash
make
```

This compiles the interpreter and produces **`bin/melt`**. The `bin/` directory is created if it doesn’t exist.

To build with **MySQL** support (optional):

```bash
make with-mysql
```

You need the MySQL client library (e.g. `libmysqlclient-dev` on Ubuntu, `mysql` on Homebrew).

For a **smaller binary** without HTTP server, MySQL, or GUI (e.g. for embedded targets), use **`make embedded`**. This produces **`bin/melt-embedded`**. Scripts that call `listen()`, MySQL built-ins, or `imagePreview()` will get a clear runtime error.

## Install the binary (optional)

Install `melt` so you can run it from any directory:

```bash
make install
```

This installs to **`/usr/local/bin`** by default. To use a different prefix:

```bash
make install PREFIX=/usr              # system-wide (e.g. Linux)
make install PREFIX=$(HOME)/.local     # user-only (~/.local/bin; add to PATH)
```

To remove: `make uninstall` (use the same `PREFIX` if you changed it).

## Run a program

From the **project root**:

```bash
./bin/melt path/to/script.melt
```

Example:

```bash
./bin/melt examples/hello.melt
```

If you installed the binary:

```bash
melt path/to/script.melt
```

Running `./bin/melt` with **no arguments** runs a built-in demo (variables, conditionals, loops).

## Your first Melt program

Create a file `hello.melt`:

```melt
let name = "Melt";
print "Hello, ";
print name;
```

Run it:

```bash
./bin/melt hello.melt
```

Output: `Hello, Melt`

## Paths and working directory

- **Import paths** in `import "path.melt";` are relative to the **directory of the file that contains the import** (the current script’s directory when that file was loaded).
- **File paths** in built-ins like `readFile(path)` are resolved relative to the **entry script’s directory** when the path is relative (e.g. `readFile("migrations/001.sql")` from `examples/web_project_mvc/run_migrations.melt` looks under `examples/web_project_mvc/`).
- Run Melt from the **project root** when using examples or the MVC app, so that relative paths resolve correctly.
