# Built-in Functions

All built-ins are globally available in Melt. Optional builds (e.g. MySQL) add more.

## Output

| Built-in | Description |
|----------|-------------|
| `print expr;` | Statement: prints the value of `expr` to stdout followed by a newline. |

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
| `arrayCreate()` | Returns a new empty array. |
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
| `imagePreview()` | Opens a window showing the current image (blocks until window is closed). **Requires** Melt built with `make with-gui` (SDL2). Without it, throws an error. Close with window button or Escape/Q. |

Example: `examples/gui_render_demo.melt`. To use the preview window: `make with-gui` (install SDL2 first: Linux `libsdl2-dev`, macOS `brew install sdl2`).

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

### Response

| Function | Description |
|----------|-------------|
| `setResponseBody(str)` | Sets the response body to the string `str`. |
| `setResponseStatus(code)` | Sets the HTTP status code (e.g. `200`, `404`). Argument is a number. |
| `setResponseContentType(str)` | Sets the `Content-Type` header (e.g. `"text/html; charset=utf-8"`, `"application/javascript"`). |
| `setResponseHeader(name, value)` | Adds a response header (e.g. `"Location"`, `"Set-Cookie"`). Can be called multiple times. |
| `servePublic(path)` | If `path` is under `/js/`, `/css/`, or `/images/`, serves the file from the `public/` folder (relative to the script directory), sets body and `Content-Type`, and returns a truthy value. Otherwise returns falsy. Use before handling other routes to serve static assets. |

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

Available only when Melt is built with **`make with-mysql`** and the MySQL client library.

| Function | Description |
|----------|-------------|
| `mysqlConnect(host, user, password, database)` | Connects to MySQL. Returns a truthy value on success, falsy on failure. |
| `mysqlQuery(sql)` | Executes the SQL string. Returns truthy on success, falsy on failure. For `SELECT`, use `mysqlFetchRow()` to read rows. |
| `mysqlFetchRow()` | Returns the next row of the last `SELECT` as a single string with columns separated by ASCII 1 (`chr(1)`). Split with `splitString(row, chr(1))`. Returns `""` when there are no more rows. |
| `mysqlClose()` | Closes the connection and frees result. |

One connection per interpreter; open once, run queries, then close.
