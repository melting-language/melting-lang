# Melt examples

Demo programs for the Melt programming language.

## How to run

From the project root:

```bash
./bin/melt examples/hello.melt
./bin/melt examples/basics.melt
./bin/melt examples/oop.melt
./bin/melt examples/counter.melt
./bin/melt examples/multi_file/main.melt
./bin/melt examples/server.melt
./bin/melt examples/file_io.melt
./bin/melt examples/array_demo.melt
./bin/melt examples/json_demo.melt
./bin/melt examples/encryption_demo.melt
./bin/melt examples/return_demo.melt
./bin/melt examples/web_project_mvc/main.melt   # then open http://localhost:8080
./bin/melt examples/official_website_using_melt/main.melt   # official site → http://localhost:4000
echo '{"jsonrpc":"2.0","id":1,"method":"ping"}' | ./bin/melt examples/mcp_demo/main.melt   # stdio/MCP demo
```

Or from the `examples` folder (if `melt` is on your PATH or you use the path to the binary):

```bash
../bin/melt hello.melt
../bin/melt basics.melt
../bin/melt oop.melt
../bin/melt counter.melt
```

## Demos

| File | Description |
|------|-------------|
| `hello.melt` | Minimal hello world |
| `basics.melt` | Variables, conditionals, loops |
| `oop.melt` | Classes, objects, methods |
| `counter.melt` | Simple Counter class with state |
| `multi_file/` | Multiple files: import class files from `main.melt` |
| `server.melt` | HTTP server (backend): `setHandler`, `listen`, request/response built-ins |
| `mysql_example.melt` | MySQL connect/query/fetch (requires `make with-mysql`) |
| `file_io.melt` | File read/write: `readFile(path)`, `writeFile(path, content)` |
| `array_demo.melt` | Arrays: `[1,2,3]`, `arr[i]`, `arrayPush`, `arrayLength`, etc. |
| `json_demo.melt` | JSON: `jsonEncode(value)`, `jsonDecode(str)` |
| `object_create_demo.melt` | Object construction: `objectCreate()`, `obj[key]`, `obj[key] = value` for dynamic keys and JSON without classes |
| `encryption_demo.melt` | Base64 and XOR: `base64Encode`/`base64Decode`, `xorCipher(str, key)` |
| `return_demo.melt` | Method return values: `return expr;` and `return;` in methods |
| `comment_demo.melt` | Comments: `//` line and `/* */` block |
| `web_project_mvc/` | MVC-style website: model, view, controller; run `./bin/melt examples/web_project_mvc/main.melt` then open http://localhost:8080 |
| `official_website_using_melt/` | Official Melt website (MVC): Home, Documentation, About, Resource, Support, Blog; run `./bin/melt examples/official_website_using_melt/main.melt` then open http://localhost:4000 |
| `mcp_demo/` | Stdio / MCP transport: one JSON-RPC line per request on stdin, one response line on stdout; run with `echo '{"jsonrpc":"2.0","id":1}' \| ./bin/melt examples/mcp_demo/main.melt` |