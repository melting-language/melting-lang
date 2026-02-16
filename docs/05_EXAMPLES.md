# Examples

All examples are under **`examples/`**. Run them from the **project root** with:

```bash
./bin/melt examples/<path>.melt
```

(If you installed the binary: `melt examples/<path>.melt`.)

---

## Core language

| File | Description |
|------|-------------|
| **hello.melt** | Minimal: print a message. |
| **basics.melt** | Variables, conditionals, loops. |
| **oop.melt** | Classes, objects, methods, `this`, properties. |
| **counter.melt** | Simple class with state. |
| **return_demo.melt** | Method return values: `return expr;` and `return;`. |
| **comment_demo.melt** | `//` and `/* */` comments. |

---

## Multi-file and import

| Path | Description |
|------|-------------|
| **multi_file/main.melt** | Imports `point.melt` and `greeter.melt`; uses their classes. Run: `./bin/melt examples/multi_file/main.melt` |

---

## Built-ins

| File | Description |
|------|-------------|
| **file_io.melt** | `readFile(path)`, `writeFile(path, content)`. |
| **array_demo.melt** | Array literals, indexing, `arrayPush`, `arrayLength`, etc. |
| **json_demo.melt** | `jsonEncode(value)`, `jsonDecode(str)`. |
| **encryption_demo.melt** | `base64Encode`, `base64Decode`, `xorCipher(str, key)`. |

---

## HTTP server

| File | Description |
|------|-------------|
| **server.melt** | Simple HTTP server: routes `/` and `/api/hello`, uses `getRequestPath`, `getRequestMethod`, `setResponseBody`, `setResponseStatus`, `setHandler`, `listen(8080)`. |

Run then open http://localhost:8080/

---

## MySQL (optional)

| File | Description |
|------|-------------|
| **mysql_example.melt** | Connects with `mysqlConnect`, runs `mysqlQuery("SELECT ...")`, reads rows with `mysqlFetchRow()`, then `mysqlClose()`. |

Requires **`make with-mysql`** and a running MySQL instance; edit host, user, password, database in the script.

---

## MVC web project

| Path | Description |
|------|-------------|
| **web_project_mvc/main.melt** | Full MVC app: config, routes, controllers, views, public assets. Run: `./bin/melt examples/web_project_mvc/main.melt` then open http://localhost:8080/ |

See [HTTP Server & MVC Framework](04_HTTP_AND_MVC.md) and **examples/web_project_mvc/README.md** for structure and how to add routes, controllers, and migrations.

---

## Official website

| Path | Description |
|------|-------------|
| **official_website_using_melt/main.melt** | Full official Melt website (MVC): Home, Documentation, About, Resource, Support, Blog (dynamic, MySQL). Port 4000. Run: `./bin/melt examples/official_website_using_melt/main.melt` then open http://localhost:4000/ |

See [HTTP Server & MVC Framework — Official website](04_HTTP_AND_MVC.md#official-website-official_website_using_melt) for full structure, blog, migrations, and menu. See **examples/official_website_using_melt/README.md** for a short overview.

---

## Running from the examples directory

If you run from inside **examples/** (and `bin/melt` is in the parent directory):

```bash
../bin/melt hello.melt
../bin/melt multi_file/main.melt
```

Imports and relative paths are still resolved relative to each file’s directory, but the “entry script” directory will be e.g. `examples/` or `examples/multi_file/`, so some paths may differ. Prefer running from the project root for the MVC app and migrations.
