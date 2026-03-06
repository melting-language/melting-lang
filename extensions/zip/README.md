# Zip extension for Melt

Create, extract, list, and read ZIP archives. Requires **libzip** (e.g. `brew install libzip`, `apt install libzip-dev`).

## Enable

In `melt.ini` or `melt.config`:

```ini
extension = zip
```

The extension is only built when libzip is found at configure time.

## Functions

| Function | Description |
|----------|-------------|
| `zipVersion()` | Extension version string. |
| `zipCreate(zipPath, paths)` | Create a new ZIP at `zipPath`. `paths` is an array of file paths to add (entry name = basename of each path). Overwrites existing archive. |
| `zipExtract(zipPath, destDir)` | Extract all entries from `zipPath` into `destDir`. Creates subdirectories as needed. Returns truthy on success. |
| `zipList(zipPath)` | Return an array of entry names (strings) in the archive. |
| `zipReadEntry(zipPath, entryName)` | Read one entry’s contents as a string (binary-safe). Returns `""` on error or if not found. |
| `zipAddFiles(zipPath, paths)` | Add files to an existing ZIP (or create if missing). `paths` is an array of file paths. Overwrites same name. |

Paths are relative to the current working directory unless absolute. Entry names in the ZIP use the file’s basename (no directory structure in the archive for `zipCreate`/`zipAddFiles`).

## Example

```melt
// Create a zip
let files = [ "readme.txt", "data.json" ];
zipCreate("out.zip", files);

// List contents
let names = zipList("out.zip");
print arrayLength(names);
let i = 0;
while (i < arrayLength(names)) {
    print arrayGet(names, i);
    i = i + 1;
}

// Read one entry
let content = zipReadEntry("out.zip", "readme.txt");
print content;

// Extract all
zipExtract("out.zip", "extracted");
```
