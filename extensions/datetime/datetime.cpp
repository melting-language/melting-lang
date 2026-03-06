// Date and time extension for Melt.
// Enable in melt.ini: extension = datetime
// Functions: timestamp, currentDate, format, parse, add, diff, extract components, timezone.

#include "interpreter.hpp"
#include <chrono>
#include <cmath>
#include <cstring>
#include <ctime>
#include <sstream>
#include <string>

using Clock = std::chrono::system_clock;
using Sec = std::chrono::seconds;
using Ms = std::chrono::milliseconds;

static std::string asString(const Value& v) {
    if (auto* s = std::get_if<std::string>(&v)) return *s;
    if (auto* d = std::get_if<double>(&v)) return std::to_string(static_cast<long long>(*d));
    if (auto* b = std::get_if<bool>(&v)) return *b ? "true" : "false";
    return "";
}

static double asNumber(const Value& v) {
    if (auto* d = std::get_if<double>(&v)) return *d;
    if (auto* s = std::get_if<std::string>(&v)) {
        try { return std::stod(*s); } catch (...) {}
    }
    return 0.0;
}

static std::tm timeToTm(std::time_t t) {
    std::tm tm{};
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    return tm;
}

static std::time_t tmToTime(std::tm& tm) {
    return std::mktime(&tm);
}

// dateVersion() — extension version
static Value dateVersion(Interpreter*, std::vector<Value>) {
    return Value(std::string("1.0"));
}

// dateTimestamp() — current Unix timestamp in seconds
static Value dateTimestamp(Interpreter*, std::vector<Value>) {
    auto now = Clock::now();
    auto sec = std::chrono::duration_cast<Sec>(now.time_since_epoch()).count();
    return Value(static_cast<double>(sec));
}

// dateTimestampMs() — current Unix timestamp in milliseconds
static Value dateTimestampMs(Interpreter*, std::vector<Value>) {
    auto now = Clock::now();
    auto ms = std::chrono::duration_cast<Ms>(now.time_since_epoch()).count();
    return Value(static_cast<double>(ms));
}

// dateNow() — current date/time as ISO 8601 string (local time)
static Value dateNow(Interpreter*, std::vector<Value>) {
    std::time_t t = Clock::to_time_t(Clock::now());
    std::tm tm = timeToTm(t);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);
    return Value(std::string(buf));
}

// dateCurrentDate() — current date only YYYY-MM-DD
static Value dateCurrentDate(Interpreter*, std::vector<Value>) {
    std::time_t t = Clock::to_time_t(Clock::now());
    std::tm tm = timeToTm(t);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", &tm);
    return Value(std::string(buf));
}

// dateCurrentTime() — current time only HH:MM:SS
static Value dateCurrentTime(Interpreter*, std::vector<Value>) {
    std::time_t t = Clock::to_time_t(Clock::now());
    std::tm tm = timeToTm(t);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm);
    return Value(std::string(buf));
}

// dateFormat(timestamp [, format]) — format Unix timestamp; default "%Y-%m-%d %H:%M:%S"
static Value dateFormat(Interpreter*, std::vector<Value> args) {
    if (args.empty()) return Value(std::string(""));
    std::time_t t = static_cast<std::time_t>(asNumber(args[0]));
    std::string fmt = "%Y-%m-%d %H:%M:%S";
    if (args.size() >= 2) fmt = asString(args[1]);
    std::tm tm = timeToTm(t);
    char buf[256];
    if (std::strftime(buf, sizeof(buf), fmt.c_str(), &tm) == 0)
        return Value(std::string(""));
    return Value(std::string(buf));
}

// dateParse(dateString) — parse ISO-like date to Unix timestamp; returns 0 on failure
static Value dateParse(Interpreter*, std::vector<Value> args) {
    if (args.empty()) return Value(0.0);
    std::string s = asString(args[0]);
    if (s.empty()) return Value(0.0);
    int y = 1970, mo = 1, d = 1, h = 0, mi = 0, sec = 0;
    char dash, t, colon;
    std::istringstream is(s);
    if (!(is >> y >> dash >> mo >> dash >> d)) return Value(0.0);
    if (is >> t && t == 'T' && (is >> h >> colon >> mi >> colon >> sec)) {}
    std::tm tm{};
    tm.tm_year = y - 1900;
    tm.tm_mon = mo - 1;
    tm.tm_mday = d;
    tm.tm_hour = h;
    tm.tm_min = mi;
    tm.tm_sec = sec;
    tm.tm_isdst = -1;
    std::time_t t_val = tmToTime(tm);
    if (t_val == static_cast<std::time_t>(-1)) return Value(0.0);
    return Value(static_cast<double>(t_val));
}

// dateAdd(timestamp, amount, unit) — unit: "second","minute","hour","day","month","year"
static Value dateAdd(Interpreter*, std::vector<Value> args) {
    if (args.size() < 3) return Value(0.0);
    std::time_t t = static_cast<std::time_t>(asNumber(args[0]));
    double amount = asNumber(args[1]);
    std::string unit = asString(args[2]);
    std::tm tm = timeToTm(t);
    if (unit == "second") {
        t += static_cast<std::time_t>(amount);
        return Value(static_cast<double>(t));
    }
    if (unit == "minute") {
        t += static_cast<std::time_t>(amount * 60);
        return Value(static_cast<double>(t));
    }
    if (unit == "hour") {
        t += static_cast<std::time_t>(amount * 3600);
        return Value(static_cast<double>(t));
    }
    if (unit == "day") {
        t += static_cast<std::time_t>(amount * 86400);
        return Value(static_cast<double>(t));
    }
    if (unit == "month") {
        int n = static_cast<int>(amount);
        tm.tm_mon += n;
        std::time_t r = tmToTime(tm);
        if (r != static_cast<std::time_t>(-1)) return Value(static_cast<double>(r));
    }
    if (unit == "year") {
        int n = static_cast<int>(amount);
        tm.tm_year += n;
        std::time_t r = tmToTime(tm);
        if (r != static_cast<std::time_t>(-1)) return Value(static_cast<double>(r));
    }
    return Value(static_cast<double>(t));
}

// dateDiff(timestamp1, timestamp2, unit) — returns (timestamp2 - timestamp1) in unit
static Value dateDiff(Interpreter*, std::vector<Value> args) {
    if (args.size() < 3) return Value(0.0);
    std::time_t t1 = static_cast<std::time_t>(asNumber(args[0]));
    std::time_t t2 = static_cast<std::time_t>(asNumber(args[1]));
    std::string unit = asString(args[2]);
    double diff = static_cast<double>(t2 - t1);
    if (unit == "second") return Value(diff);
    if (unit == "minute") return Value(diff / 60.0);
    if (unit == "hour") return Value(diff / 3600.0);
    if (unit == "day") return Value(diff / 86400.0);
    return Value(diff);
}

// dateYear(timestamp), dateMonth(timestamp), ...
static Value dateYear(Interpreter*, std::vector<Value> args) {
    if (args.empty()) return Value(0.0);
    std::time_t t = static_cast<std::time_t>(asNumber(args[0]));
    std::tm tm = timeToTm(t);
    return Value(static_cast<double>(tm.tm_year + 1900));
}
static Value dateMonth(Interpreter*, std::vector<Value> args) {
    if (args.empty()) return Value(0.0);
    std::time_t t = static_cast<std::time_t>(asNumber(args[0]));
    std::tm tm = timeToTm(t);
    return Value(static_cast<double>(tm.tm_mon + 1));
}
static Value dateDay(Interpreter*, std::vector<Value> args) {
    if (args.empty()) return Value(0.0);
    std::time_t t = static_cast<std::time_t>(asNumber(args[0]));
    std::tm tm = timeToTm(t);
    return Value(static_cast<double>(tm.tm_mday));
}
static Value dateHour(Interpreter*, std::vector<Value> args) {
    if (args.empty()) return Value(0.0);
    std::time_t t = static_cast<std::time_t>(asNumber(args[0]));
    std::tm tm = timeToTm(t);
    return Value(static_cast<double>(tm.tm_hour));
}
static Value dateMinute(Interpreter*, std::vector<Value> args) {
    if (args.empty()) return Value(0.0);
    std::time_t t = static_cast<std::time_t>(asNumber(args[0]));
    std::tm tm = timeToTm(t);
    return Value(static_cast<double>(tm.tm_min));
}
static Value dateSecond(Interpreter*, std::vector<Value> args) {
    if (args.empty()) return Value(0.0);
    std::time_t t = static_cast<std::time_t>(asNumber(args[0]));
    std::tm tm = timeToTm(t);
    return Value(static_cast<double>(tm.tm_sec));
}

// dateWeekday(timestamp) — 0=Sunday, 1=Monday, ... 6=Saturday
static Value dateWeekday(Interpreter*, std::vector<Value> args) {
    if (args.empty()) return Value(0.0);
    std::time_t t = static_cast<std::time_t>(asNumber(args[0]));
    std::tm tm = timeToTm(t);
    return Value(static_cast<double>(tm.tm_wday));
}

// dateZoneOffset() — local UTC offset in hours (e.g. 5.5 for IST). local = utc + offset.
static Value dateZoneOffset(Interpreter*, std::vector<Value>) {
    std::time_t t = Clock::to_time_t(Clock::now());
#if defined(_WIN32) || defined(_WIN64)
    std::tm utc{};
    gmtime_s(&utc, &t);
#else
    std::tm utc{};
    gmtime_r(&t, &utc);
#endif
    std::time_t t_utc_as_local = tmToTime(utc);
    if (t_utc_as_local == static_cast<std::time_t>(-1)) return Value(0.0);
    double diffSec = static_cast<double>(t - t_utc_as_local);
    return Value(diffSec / 3600.0);
}

// dateUtcOffsetString() — e.g. "+05:30" or "-08:00"
static Value dateUtcOffsetString(Interpreter*, std::vector<Value>) {
    std::time_t t = Clock::to_time_t(Clock::now());
#if defined(_WIN32) || defined(_WIN64)
    std::tm utc{};
    gmtime_s(&utc, &t);
#else
    std::tm utc{};
    gmtime_r(&t, &utc);
#endif
    std::time_t t_utc_as_local = tmToTime(utc);
    if (t_utc_as_local == static_cast<std::time_t>(-1)) return Value(std::string("+00:00"));
    int diffSec = static_cast<int>(t - t_utc_as_local);
    int sign = (diffSec >= 0) ? 1 : -1;
    diffSec = std::abs(diffSec);
    int h = diffSec / 3600;
    int m = (diffSec % 3600) / 60;
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%+.2d:%02d", sign * h, m);
    return Value(std::string(buf));
}

// dateStyle(timestamp, style) — style: "short", "long", "datetime", "date", "time"
static Value dateStyle(Interpreter*, std::vector<Value> args) {
    if (args.empty()) return Value(std::string(""));
    std::time_t t = static_cast<std::time_t>(asNumber(args[0]));
    std::string style = args.size() >= 2 ? asString(args[1]) : "datetime";
    std::tm tm = timeToTm(t);
    char buf[128];
    const char* fmt = "%Y-%m-%d %H:%M:%S";
    if (style == "short") fmt = "%Y-%m-%d %H:%M";
    else if (style == "long") fmt = "%A, %B %d, %Y %H:%M:%S";
    else if (style == "date") fmt = "%Y-%m-%d";
    else if (style == "time") fmt = "%H:%M:%S";
    else if (style == "datetime") fmt = "%Y-%m-%d %H:%M:%S";
    if (std::strftime(buf, sizeof(buf), fmt, &tm) == 0)
        return Value(std::string(""));
    return Value(std::string(buf));
}

extern "C" void melt_register(Interpreter* interp) {
    interp->registerBuiltin("dateVersion", dateVersion);
    interp->registerBuiltin("dateTimestamp", dateTimestamp);
    interp->registerBuiltin("dateTimestampMs", dateTimestampMs);
    interp->registerBuiltin("dateNow", dateNow);
    interp->registerBuiltin("dateCurrentDate", dateCurrentDate);
    interp->registerBuiltin("dateCurrentTime", dateCurrentTime);
    interp->registerBuiltin("dateFormat", dateFormat);
    interp->registerBuiltin("dateParse", dateParse);
    interp->registerBuiltin("dateAdd", dateAdd);
    interp->registerBuiltin("dateDiff", dateDiff);
    interp->registerBuiltin("dateYear", dateYear);
    interp->registerBuiltin("dateMonth", dateMonth);
    interp->registerBuiltin("dateDay", dateDay);
    interp->registerBuiltin("dateHour", dateHour);
    interp->registerBuiltin("dateMinute", dateMinute);
    interp->registerBuiltin("dateSecond", dateSecond);
    interp->registerBuiltin("dateWeekday", dateWeekday);
    interp->registerBuiltin("dateZoneOffset", dateZoneOffset);
    interp->registerBuiltin("dateUtcOffsetString", dateUtcOffsetString);
    interp->registerBuiltin("dateStyle", dateStyle);
}
