# image_optimize extension

Native Melt extension that adds image optimization helpers:

- `imageOptimizeVersion()` -> string version
- `optimizeImage(inputPath, outputPath, quality)` -> truthy/falsy
- `resizeImage(inputPath, outputPath, width, height, quality)` -> truthy/falsy

## Build

From project root:

```bash
make modules
```

This builds `bin/modules/image_optimize.<so|dylib|dll>`.

## Enable

In `bin/melt.ini` or project `melt.config`:

```ini
extension_dir = modules
extension = image_optimize
```

Then run Melt scripts normally.

## Notes

- On **macOS**, uses `sips` (built-in) and optionally `pngquant` for PNG optimization.
- On **Linux/Windows**, uses `magick` (ImageMagick), fallback `convert` on Linux.
- Returns falsy if required tools are unavailable or processing fails.
