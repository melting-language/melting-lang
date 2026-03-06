# Built-in Functions

All built-ins are globally available in Melt. Optional builds (e.g. MySQL) add more.

## Output

| Built-in | Description |
|----------|-------------|
| `print expr;` | Statement: prints the value of `expr` to stdout followed by a newline. |

---

## Numbers

Formatting, random numbers, and basic math helpers.

| Function | Description |
|----------|-------------|
| `numberFormat(n, decimals)` | Returns a string with `n` formatted to `decimals` decimal places (0–20). Example: <code>numberFormat(3.14159, 2)</code> → <code>"3.14"</code>. |
| `random()` | Returns a random number in the range [0, 1). |
| `random(lo, hi)` | Returns a random number in the range [lo, hi] (floating-point). |
| `randomInt(lo, hi)` | Returns a random integer in the range [lo, hi] (inclusive). |
| `round(n)` | Rounds to the nearest integer (returns a number). |
| `floor(n)` | Greatest integer ≤ n. |
| `ceil(n)` | Least integer ≥ n. |
| `abs(n)` | Absolute value of n. |
| `min(a, b)` | Returns the smaller of two numbers. |
| `max(a, b)` | Returns the larger of two numbers. |

---

## Modules and central config

**Module search:** `import "path"` resolves in order: (1) current file directory, (2) each directory in the **module path** (from `melt.config`, `melt.ini`, or `addModulePath`).

**Global config (php.ini-style):** Place **`melt.ini`** in the same directory as the `melt` binary (e.g. `build/melt.ini`). It is read first; then project **`melt.config`** (next to the entry script) is merged. Format for both: `key = value`, `#` comments.

- **melt.ini (global):** Paths are relative to the **bin** directory. Keys: `modulePath`, `extension_dir` (default `modules`), `extension` (comma-separated list of extension names to load).
- **melt.config (project):** Paths relative to the project directory. `modulePath` is **appended** to the list. Other keys overwrite global.

**Loadable extensions:** If `extension` is set (in melt.ini or melt.config), Melt loads shared libraries from `binDir/extension_dir/name.suffix` (`.so` Linux, `.dylib` macOS, `.dll` Windows) and calls **`melt_register(Interpreter*)`** in each. Extensions register built-ins via `interp->registerBuiltin("name", fn)`.

**Developing extensions:** Build a shared library that exports `extern "C" void melt_register(Interpreter* interp);` and call `interp->registerBuiltin("myFunc", ...)` for each built-in. Put the library in the `modules` directory next to the `melt` binary and add its name to `extension` in melt.ini (e.g. `extension = example`).

**Compiling extensions:** From the project root, build the interpreter and extensions with:

```bash
cmake -B build
cmake --build build
```

This produces `build/melt` and, in `build/modules/`, the extension shared libraries (e.g. `example.so` on Linux, `example.dylib` on macOS, `example.dll` on Windows). When you run `./build/melt`, the interpreter looks for extensions in `build/modules/` by default (`extension_dir = modules`). Create `build/melt.ini` (or a project `melt.config`) and set `extension = example` (or `os`, `ffmpeg`, `headless_browser`, `image_optimize`, `datetime`) to load an extension. The **datetime** extension provides `dateTimestamp`, `dateNow`, `dateFormat`, `dateParse`, `dateAdd`, `dateDiff`, `dateCurrentDate`, `dateCurrentTime`, component getters, and timezone helpers — see `extensions/datetime/README.md`. To add your own extension, add a new `add_melt_extension(myname path/to/myname.cpp)` in `CMakeLists.txt` and implement `melt_register` in your source.

| Function | Description |
|----------|-------------|
| `addModulePath(dir)` | Adds a directory to the module search path (relative to current script dir). |
| `getConfig(key)` | Returns value of `key` from melt.ini / melt.config, or `""`. |

Example: **examples/module_config_demo/**.

---

## File I/O

Paths in `readFile` / `writeFile` are resolved relative to the **entry script’s directory** when the path is relative.

| Function | Description |
|----------|-------------|
| `readFile(path)` | Reads the file at `path`. Returns file contents as a string, or `""` on error. |
| `writeFile(path, content)` | Writes `content` (string) to `path`. Returns a truthy value on success, falsy on failure. |

---

## Arrays

| Function | Description |
|----------|-------------|
| `arrayCreate()` / `arrayCreate(v1, v2, ...)` | Returns a new array; with arguments, initializes with those elements (e.g. closures). |
| `arrayPush(arr, value)` | Appends `value` to `arr`. Returns the new length (number). |
| `arrayGet(arr, i)` | Returns the element at index `i` (0-based). Returns a falsy value if out of range. |
| `arraySet(arr, i, value)` | Sets `arr[i] = value`; resizes if needed. Returns a truthy value. |
| `arrayLength(arr)` | Returns the number of elements in `arr` (0 if not an array). |

You can also use literal `[a, b, c]`, `arr[i]`, and `arr[i] = value` in syntax.

---

## Vectors (math)

2D and 3D math vectors: a dedicated type (not arrays). Create with `vectorCreate2(x, y)` or `vectorCreate3(x, y, z)`; use built-ins for operations. `vectorCross` is for 3D only. Components are numbers; missing arguments default to 0.

| Function | Description |
|----------|-------------|
| `vectorCreate2(x, y)` | Returns a 2D vector (x, y). |
| `vectorCreate3(x, y, z)` | Returns a 3D vector (x, y, z). |
| `vectorAdd(a, b)` | Returns a + b (same dimension). |
| `vectorSub(a, b)` | Returns a − b (same dimension). |
| `vectorScale(v, s)` | Returns v scaled by number s. |
| `vectorLength(v)` | Returns the length (magnitude) of v. |
| `vectorDot(a, b)` | Returns the dot product (same dimension). |
| `vectorCross(a, b)` | Returns the cross product (3D only). |
| `vectorX(v)` `vectorY(v)` `vectorZ(v)` | Returns the x, y, or z component (z is 0 for 2D). |
| `vectorDim(v)` | Returns 2 or 3. |

Example: `examples/vector_demo.melt`. Vectors print as `(x, y)` or `(x, y, z)`; `jsonEncode(vector)` yields `[x, y]` or `[x, y, z]`.

---

## GUI render (image buffer)

Draw into an RGB image buffer and save as a PPM file (open in a viewer or convert to PNG with ImageMagick: `convert out.ppm out.png`). One image per interpreter; path is resolved relative to the script directory.

| Function | Description |
|----------|-------------|
| `imageCreate(width, height)` | Creates an image (1–8192). Clears any previous image. |
| `imageFill(r, g, b)` | Fills the image with RGB (0–255). Or `imageFill(gray)` for grayscale. |
| `imageSetPixel(x, y, r, g, b)` | Sets one pixel. Or `imageSetPixel(x, y, gray)`. |
| `imageDrawLine(x1, y1, x2, y2, r, g, b)` | Draws a line (Bresenham). r,g,b optional (default white). |
| `imageSavePpm(path)` | Saves the image as PPM P6. Path relative to script dir. Returns truthy on success. |
| `imagePreview()` | Opens a window showing the current image (blocks until window is closed). **Requires** Melt built with default build (SDL2) (SDL2). Without it, throws an error. Close with window button or Escape/Q. |

Example: `examples/gui_render_demo.melt`. To use the preview window: default build (SDL2) (install SDL2 first: Linux `libsdl2-dev`, macOS `brew install sdl2`).

---

## Strings

| Function | Description |
|----------|-------------|
| `splitString(str, sep)` | Splits `str` by separator `sep` and returns an array of strings. Empty `sep` splits into single characters. |
| `replaceString(str, from, to)` | Replaces all occurrences of `from` with `to` in `str`. Returns the new string. |
| `escapeHtml(str)` | Escapes `&`, `<`, `>`, `"` for HTML. Use when outputting user content to avoid XSS. |
| `urlDecode(str)` | Decodes URL-encoded string: `%XX` → character, `+` → space. Use for form query/body values. |
| `chr(n)` | Returns a one-character string with ASCII code `n` (0–255). Use e.g. `chr(1)` to split `mysqlFetchRow()` output. |

---

## JSON

| Function | Description |
|----------|-------------|
| `jsonEncode(value)` | Converts a Melt value to a JSON string. Handles numbers, strings, booleans, arrays, objects. |
| `jsonDecode(str)` | Parses a JSON string and returns a Melt value. Arrays become Melt arrays; objects become Melt objects with field access (e.g. `obj.field`). |
| `objectCreate()` | Returns a new empty object. Set properties with `obj.field = value` or `obj[key] = value` (when `key` is a string), then use `jsonEncode(obj)` to build JSON without defining a class. |

**Dynamic property access:** Use `obj[key]` to read and `obj[key] = value` to write when `key` is a string (e.g. a variable). This lets you build or read JSON-style objects with dynamic keys. For missing keys, `obj[key]` returns a falsy value.

---

## Encoding / simple crypto

| Function | Description |
|----------|-------------|
| `base64Encode(str)` | Returns the Base64-encoded string. |
| `base64Decode(str)` | Decodes a Base64 string; returns the decoded string. |
| `xorCipher(data, key)` | XORs `data` with `key` (repeating key). Same function and key for both “encrypt” and “decrypt”. For obfuscation only, not secure crypto. |

---

## Blade-like views (templates)

| Function | Description |
|----------|-------------|
| `renderView(templatePath, data)` | Loads an HTML template file at `templatePath` (relative to the script directory). Replaces `{{ key }}` with the escaped value of `data.key`, and `{!! key !!}` with the raw value. `data` is a Melt object whose fields are the keys. Returns the rendered string. |

Used in the MVC framework; see [HTTP Server & MVC Framework](04_HTTP_AND_MVC.md).

---

## HTTP server

Only meaningful when the script is used as an HTTP server (e.g. after `setHandler(...)` and `listen(port)`). Each request runs the handler once; these built-ins refer to the current request/response.

### Request

| Function | Description |
|----------|-------------|
| `getRequestPath()` | Returns the request path (e.g. `"/"`, `"/items"`). |
| `getRequestMethod()` | Returns the HTTP method (e.g. `"GET"`, `"POST"`). |
| `getRequestBody()` | Returns the raw request body as a string. |
| `getRequestHeader(name)` | Returns the value of the request header (e.g. `"Cookie"`). Case-insensitive. Empty string if missing. |
| `uploadFileName(fieldName)` | For `multipart/form-data`, returns uploaded filename for the given field (e.g. `"file"`), or `""` if missing. |
| `uploadFileData(fieldName)` | For `multipart/form-data`, returns uploaded file bytes as a string for the given field, or `""`. |
| `uploadSave(fieldName, path)` | For `multipart/form-data`, saves uploaded file bytes from field to `path` (relative to current script dir). Returns truthy/falsy. |

### Response

| Function | Description |
|----------|-------------|
| `setResponseBody(str)` | Sets the response body to the string `str`. If streaming has already started (see `streamChunk`), appends instead of replacing so the full body is still available for tests/logging. |
| `setResponseStatus(code)` | Sets the HTTP status code (e.g. `200`, `404`). Argument is a number. |
| `setResponseContentType(str)` | Sets the `Content-Type` header (e.g. `"text/html; charset=utf-8"`, `"application/javascript"`). |
| `setResponseHeader(name, value)` | Adds a response header (e.g. `"Location"`, `"Set-Cookie"`). Can be called multiple times. |
| `streamChunk(str)` | Appends `str` to the response body and, when handling an HTTP request, sends that chunk immediately to the client (chunked transfer). Use for streaming responses (e.g. Server-Sent Events or progressive output). If streaming is not active (e.g. not in an HTTP request), only appends to the in-memory body. Once any chunk is sent, the response uses `Transfer-Encoding: chunked`; redirects or status changes after the first chunk have no effect. Use `sleep(seconds)` between `streamChunk` calls to add a delay (e.g. `sleep(0.5)` for 0.5 seconds). |
| `sleep(seconds)` | Pauses execution for the given number of seconds (can be fractional, e.g. `0.5` for half a second). Useful between `streamChunk` calls to throttle streaming. |
| `servePublic(path)` | If `path` is under `/js/`, `/css/`, or `/images/`, serves the file from the `public/` folder (relative to the script directory), sets body and `Content-Type`, and returns a truthy value. Otherwise returns falsy. Use before handling other routes to serve static assets. |

**Streaming responses:** Call `streamChunk(str)` to send parts of the body before the handler returns. The server uses `Transfer-Encoding: chunked` and sends each chunk as it arrives. Set `Content-Type: text/event-stream` and use `streamChunk("data: ...\n\n")` for [Server-Sent Events](https://html.spec.whatwg.org/multipage/server-sent-events.html). Once the first chunk is sent, the response is committed (redirects and status changes have no effect). If you use both `setResponseBody` and `streamChunk`, the first `streamChunk` turns on streaming; later `setResponseBody` calls append to the in-memory buffer only.

### Server control

| Function | Description |
|----------|-------------|
| `setHandler("ClassName")` | Sets the request handler to the class named `ClassName`. The class must define `method handle()`. |
| `listen(port)` | Starts the HTTP server on the given port (number). Blocks; for each request, sets request data, calls the handler’s `handle()`, then sends the response. |

---

## Stdio / MCP transport

For stdio-based servers (e.g. [MCP](https://modelcontextprotocol.io)): read one message per line from stdin, pass to a handler, write one response line to stdout. Set the handler with `setMcpHandler("ClassName")` then call `runMcp()`. The handler class must define `method handle()`. Inside, use `getMcpRequest()` to get the raw line (e.g. a JSON-RPC message), `jsonDecode` / `jsonEncode` as needed, then `setMcpResponse(responseString)`.

| Function | Description |
|----------|-------------|
| `readStdinLine()` | Reads one line from stdin. Returns the line as a string, or `""` on EOF. |
| `writeStdout(str)` | Writes `str` to stdout with no newline; flushes so output is sent immediately. |
| `writeStderr(str)` | Writes `str` to stderr with no newline; use for logging (e.g. MCP allows stderr for server logs). |
| `setMcpRequest(str)` | Sets the current MCP request line (used internally by `runMcp`). |
| `getMcpRequest()` | Returns the current MCP request line (raw string). |
| `setMcpResponse(str)` | Sets the MCP response string to send back (handler must call this with e.g. `jsonEncode(result)`). |
| `getMcpResponse()` | Returns the current MCP response string. |
| `setMcpHandler("ClassName")` | Sets the MCP handler to the class named `ClassName`. The class must define `method handle()`. |
| `runMcp()` | Blocks in a loop: reads a line from stdin, sets request, calls the MCP handler’s `handle()`, writes the response line to stdout. Exits when stdin is closed. |

Example: `examples/mcp_server.melt`. Run with e.g. `echo '{"jsonrpc":"2.0","id":1,"method":"ping"}' | ./bin/melt examples/mcp_server.melt`.

---

## MySQL (optional)

Available only when Melt is built with **`-DUSE_MYSQL=ON`** and the MySQL client library.

| Function | Description |
|----------|-------------|
| `mysqlConnect(host, user, password, database)` | Connects to MySQL. Returns a truthy value on success, falsy on failure. |
| `mysqlQuery(sql)` | Executes the SQL string. Returns truthy on success, falsy on failure. For `SELECT`, use `mysqlFetchRow()` to read rows. |
| `mysqlFetchRow()` | Returns the next row of the last `SELECT` as a single string with columns separated by ASCII 1 (`chr(1)`). Split with `splitString(row, chr(1))`. Returns `""` when there are no more rows. |
| `mysqlFetchAll()` | Returns all remaining rows of the last `SELECT` as an array of rows (`[["c1","c2"], ...]`). |
| `mysqlClose()` | Closes the connection and frees result. |

One connection per interpreter; open once, run queries, then close.

---

## SQLite (optional)

Available only when Melt is built with default build (SQLite included).

| Function | Description |
|----------|-------------|
| `sqliteOpen(path)` | Opens/creates a SQLite database file. Returns truthy/falsy. |
| `sqliteExec(sql)` | Executes SQL that does not need row fetching (e.g. `CREATE`, `INSERT`, `UPDATE`, `DELETE`). |
| `sqliteQuery(sql)` | Prepares a query for row-by-row reading. Returns truthy/falsy. |
| `sqliteFetchRow()` | Returns next row as one string with columns separated by `chr(1)`. Returns `""` when done. |
| `sqliteFetchAll()` | Returns all remaining rows of the prepared query as an array of rows (`[["c1","c2"], ...]`). |
| `sqliteClose()` | Finalizes active statement and closes the database. |

Like MySQL, split rows with `splitString(row, chr(1))`.
