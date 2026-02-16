# Multi-file demo: import classes

This folder shows how to split Melt code into multiple files and import class definitions.

## Layout

- **point.melt** – defines the `Point` class
- **greeter.melt** – defines the `Greeter` class  
- **main.melt** – imports both and uses them

## Syntax

```melt
import "path/to/file.melt";
```

The path is relative to the directory of the file that contains the `import` (so relative to this folder when running `main.melt`).

## Run

From the project root:

```bash
./melt examples/multi_file/main.melt
```

Expected output: Point(3,4), then Point(13,24), then "Hello, Melt".
