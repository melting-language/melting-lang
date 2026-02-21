# headless_browser extension

Headless browser automation built-ins for Melt (Chromium/Chrome/Edge CLI mode):

- `headlessBrowserVersion()`
- `browserAvailable()` -> truthy when a supported browser binary is found
- `browserScreenshot(url, outputPath, width?, height?)`
- `browserDumpDom(url, outputPath)`
- `browserPdf(url, outputPath)`

## Build

From project root:

```bash
make modules
```

This builds `bin/modules/headless_browser.<so|dylib|dll>`.

## Enable

In `bin/melt.ini` or project `melt.config`:

```ini
extension_enabled = 1
extension_dir = modules
extension = headless_browser
```

Or combine with others:

```ini
extension = example,image_optimize,os,headless_browser
```

## Notes

- Requires a Chromium-compatible browser installed.
- On macOS, default app bundle paths are checked first.
- On Linux/Windows, looks for binaries in PATH (`google-chrome`, `chromium`, `msedge`, etc.).
