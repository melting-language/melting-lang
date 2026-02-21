# FFmpeg extension for Melt

Loadable extension that wraps **ffmpeg** and **ffprobe** so Melt scripts can convert media, extract audio, get duration, and probe format/streams.

## Requirements

- **ffmpeg** on PATH (for convert, extract audio, to GIF).
- **ffprobe** on PATH (for duration and info; usually shipped with ffmpeg).

Install: `brew install ffmpeg` (macOS), `apt install ffmpeg` (Debian/Ubuntu), or download from [ffmpeg.org](https://ffmpeg.org).

## Build

From repo root:

```bash
make modules
```

This builds `bin/modules/ffmpeg.so` (Linux), `ffmpeg.dylib` (macOS), or `ffmpeg.dll` (Windows).

## Enable

In `bin/melt.ini` or project `melt.config`:

```ini
extension_enabled = 1
extension_dir = modules
extension = ffmpeg
```

Or comma-separated: `extension = os,ffmpeg`.

## Built-ins

| Function | Description |
|----------|-------------|
| `ffmpegVersion()` | Extension version string (e.g. `"1.0"`). |
| `ffmpegAvailable()` | True if `ffmpeg` is on PATH. |
| `ffprobeAvailable()` | True if `ffprobe` is on PATH. |
| `ffmpegConvert(inputPath, outputPath [, extraOptions])` | Transcode; optional extra args (e.g. `"-b:v 1M"`). Returns truthy on success. |
| `ffmpegExtractAudio(inputPath, outputPath [, codec])` | Extract audio; `codec`: `"mp3"` (default), `"aac"`, `"opus"`. |
| `ffmpegToGif(inputPath, outputPath [, width])` | Video to animated GIF; optional width (default 320). |
| `ffprobeDuration(inputPath)` | Duration in seconds (number), or `-1` on error. |
| `ffprobeInfo(inputPath)` | JSON string with format and streams; use `jsonDecode()` in Melt. |
| `ffmpegGenerateTestVideo(outputPath [, duration [, width [, height]]])` | Generate a short test video (lavfi testsrc). Defaults: 2 s, 320×240. |

## Example

See `examples/ffmpeg_demo.melt`. Quick usage:

```melt
if (!ffmpegAvailable()) {
    print "Install ffmpeg (e.g. brew install ffmpeg)";
} else {
    let ok = ffmpegConvert("input.mp4", "output.webm");
    print "Convert: " + ok;
    let sec = ffprobeDuration("input.mp4");
    print "Duration (s): " + sec;
}
```
