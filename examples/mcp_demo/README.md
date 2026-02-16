# MCP / Stdio transport demo

This folder demonstrates Melt’s **stdio transport** for MCP-style servers: one message per line on **stdin**, one response per line on **stdout**.

## Run

From the **project root**:

```bash
# Single request (response printed to stdout)
echo '{"jsonrpc":"2.0","id":1,"method":"ping"}' | ./bin/melt examples/mcp_demo/main.melt

# Multiple requests (one line per request; close stdin to exit)
printf '%s\n%s\n' '{"jsonrpc":"2.0","id":1,"method":"ping"}' '{"jsonrpc":"2.0","id":2}' | ./bin/melt examples/mcp_demo/main.melt
```

Logs go to **stderr** (e.g. `[mcp_demo] received line`). Redirect stderr if you only want the JSON response:

```bash
echo '{"jsonrpc":"2.0","id":1}' | ./bin/melt examples/mcp_demo/main.melt 2>/dev/null
```

## What it does

- **main.melt** defines a class `McpServer` with `method handle()`.
- The handler calls `getMcpRequest()` to read the current line, `jsonDecode(raw)` to parse it, then `setMcpResponse(jsonEncode(response))` to send the response.
- `setMcpHandler("McpServer")` and `runMcp()` start the loop: read line → call handler → write response line.

See the [Stdio / MCP transport](../../docs/03_BUILTINS.md#stdio--mcp-transport) section in the docs and the [architecture diagram](../../docs/html/index.html) in the HTML documentation.
