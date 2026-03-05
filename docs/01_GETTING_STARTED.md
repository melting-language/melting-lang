# Getting Started

## Build

From the project root:

```bash
cmake -B build
cmake --build build
```

This compiles the interpreter and produces **`build/melt`**. The default build includes SQLite, SDL2 (GUI), and QR. Optional **MySQL** support:

```bash
cmake -B build -DUSE_MYSQL=ON
cmake --build build
```

You need the MySQL client library (e.g. `libmysqlclient-dev` on Ubuntu, `mysql` on Homebrew).

## Install the binary (optional)

Install `melt` so you can run it from any directory:

```bash
cmake --install build
```

This installs to **`/usr/local`** by default. To use a different prefix:

```bash
cmake --install build --prefix /usr
cmake --install build --prefix $(HOME)/.local   # user-only (~/.local/bin; add to PATH)
```

You can also use the Makefile wrapper: `make install` or `make install PREFIX=/usr`.

## Run a program

From the **project root**:

```bash
./build/melt path/to/script.melt
```

Example:

```bash
./build/melt examples/hello.melt
```

If you installed the binary:

```bash
melt path/to/script.melt
```

Running `./build/melt` with **no arguments** runs a built-in demo (variables, conditionals, loops).

## Your first Melt program

Create a file `hello.melt`:

```melt
let name = "Melt";
print "Hello, ";
print name;
```

Run it:

```bash
./build/melt hello.melt
```

Output: `Hello, Melt`

## Paths and working directory

- **Import paths** in `import "path.melt";` are relative to the **directory of the file that contains the import** (the current script's directory when that file was loaded).
- **File paths** in built-ins like `readFile(path)` are resolved relative to the **entry script's directory** when the path is relative (e.g. `readFile("migrations/001.sql")` from `examples/web_project_mvc/run_migrations.melt` looks under `examples/web_project_mvc/`).
- Run Melt from the **project root** when using examples or the MVC app, so that relative paths resolve correctly.
