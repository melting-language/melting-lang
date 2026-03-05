# os extension

Adds OS utility built-ins for Melt:

- `osVersion()` -> extension version string
- `osName()` -> `"macos"`, `"linux"`, `"windows"`, or `"unknown"`
- `osArch()` -> `"arm64"`, `"x64"`, `"x86"`, or `"unknown"`
- `osPwd()` -> current working directory
- `osGetEnv(name)` -> environment variable value or `""`
- `osExec(command)` -> system command exit code (number)
- `osFileExists(path)` -> truthy/falsy
- `osMkdirs(path)` -> recursively create directories (truthy/falsy)

## Build

From project root:

```bash
cmake -B build
cmake --build build
```

This builds `build/modules/os.<so|dylib|dll>`.

## Enable

In `build/melt.ini` (next to `build/melt`) or project `melt.config`:

```ini
extension_enabled = 1
extension_dir = modules
extension = os
```

Multiple extensions can be loaded with comma-separated names, e.g.:

```ini
extension = example,image_optimize,os
```
