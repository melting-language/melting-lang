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
make modules
```

This builds `bin/modules/os.<so|dylib|dll>`.

## Enable

In `bin/melt.ini` or project `melt.config`:

```ini
extension_enabled = 1
extension_dir = modules
extension = os
```

Multiple extensions can be loaded with comma-separated names, e.g.:

```ini
extension = example,image_optimize,os
```
