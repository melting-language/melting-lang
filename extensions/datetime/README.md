# DateTime extension for Melt

Date and time functions: timestamps, current date/time, format, parse, add, diff, components, timezone.

## Enable

In `melt.ini` or `melt.config`:

```ini
extension = datetime
```

Build places `datetime.so` (or `datetime.dylib` / `datetime.dll`) in `build/modules/`.

## Functions

| Function | Description |
|----------|-------------|
| `dateVersion()` | Extension version string. |
| `dateTimestamp()` | Current Unix timestamp (seconds since 1970-01-01 UTC). |
| `dateTimestampMs()` | Current Unix timestamp in milliseconds. |
| `dateNow()` | Current local date/time as ISO string `YYYY-MM-DDTHH:MM:SS`. |
| `dateCurrentDate()` | Current date only `YYYY-MM-DD`. |
| `dateCurrentTime()` | Current time only `HH:MM:SS`. |
| `dateFormat(timestamp [, format])` | Format a Unix timestamp. Default format `%Y-%m-%d %H:%M:%S`. Use [strftime](https://en.cppreference.com/w/cpp/chrono/c/strftime) placeholders. |
| `dateParse(dateString)` | Parse ISO-like date (`YYYY-MM-DD` or `YYYY-MM-DDTHH:MM:SS`) to Unix timestamp. Returns 0 on failure. |
| `dateAdd(timestamp, amount, unit)` | Add time. `unit`: `"second"`, `"minute"`, `"hour"`, `"day"`, `"month"`, `"year"`. |
| `dateDiff(timestamp1, timestamp2, unit)` | Difference (t2 - t1) in given unit. |
| `dateYear(timestamp)` | Year (4 digits). |
| `dateMonth(timestamp)` | Month 1–12. |
| `dateDay(timestamp)` | Day of month 1–31. |
| `dateHour(timestamp)` | Hour 0–23. |
| `dateMinute(timestamp)` | Minute 0–59. |
| `dateSecond(timestamp)` | Second 0–59. |
| `dateWeekday(timestamp)` | Day of week 0 (Sunday) – 6 (Saturday). |
| `dateZoneOffset()` | Local UTC offset in hours (e.g. 5.5 for IST). |
| `dateUtcOffsetString()` | Offset as string (e.g. `"+05:30"`, `"-08:00"`). |
| `dateStyle(timestamp, style)` | Format by style: `"short"`, `"long"`, `"datetime"`, `"date"`, `"time"`. |

All times are in **local** time except raw timestamps (Unix seconds UTC).
