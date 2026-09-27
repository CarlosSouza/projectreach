/* HaloPad Windows runtime: kernel32 time, locale, messages and process odds and ends (G3).
 *
 * Time: SYSTEMTIME/FILETIME from the host clock and time zone, with the zone's daylight
 * rules for the current year found from the host (GetTimeZoneInformation).
 * Locale: HaloPad presents one locale, English (United States), LCID 0x409, code page
 * 1252, as a US Windows XP does: GetLocaleInfo, date and time formatting with its
 * pictures, EnumSystemLocalesA, and CompareString with Windows' word sort (case second,
 * hyphen and apostrophe ignored unless SORT_STRINGSORT). Other locales stop with their
 * number.
 * Messages: FormatMessageA from the system table for the Win32 errors HaloPad produces;
 * an unknown message is ERROR_MR_MID_NOT_FOUND, as on Windows.
 * Also GlobalMemoryStatus (a 2 GB process, as a large-address-unaware program sees),
 * IsBad*Ptr from the guest memory map, SetErrorMode, priority class, TerminateProcess. */
#include "halopad_win32.h"
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <sys/sysctl.h>

uint32_t halopad_call_guest(uint32_t va, uint32_t nargs, const uint32_t *args);
uint32_t VirtualQuery_c(uint32_t address, uint32_t info, uint32_t length);
uint32_t LocalAlloc_c(uint32_t flags, uint32_t size);

#define LCID_US 0x409u
static uint32_t err(uint32_t e) { halopad_last_error = e; return 0; }

/* ---- SYSTEMTIME and FILETIME ---- */

static void put_st(uint32_t st, const struct tm *t, int ms)
{
    uint16_t v[8] = {(uint16_t)(t->tm_year + 1900), (uint16_t)(t->tm_mon + 1), (uint16_t)t->tm_wday, (uint16_t)t->tm_mday,
                     (uint16_t)t->tm_hour, (uint16_t)t->tm_min, (uint16_t)t->tm_sec, (uint16_t)ms};
    memcpy(G(st), v, 16);
}
static void get_st(uint32_t st, uint16_t v[8]) { memcpy(v, G(st), 16); }

uint32_t GetSystemTime_c(uint32_t st)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm t;
    gmtime_r(&ts.tv_sec, &t);
    put_st(st, &t, (int)(ts.tv_nsec / 1000000));
    return 0;
}

uint32_t GetLocalTime_c(uint32_t st)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm t;
    localtime_r(&ts.tv_sec, &t);
    put_st(st, &t, (int)(ts.tv_nsec / 1000000));
    return 0;
}

static int days_in(int y, int m) { static const int d[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; return d[m - 1] + (m == 2 && ((y % 4 == 0 && y % 100) || y % 400 == 0)); }

uint32_t SystemTimeToFileTime_c(uint32_t st, uint32_t ft)
{
    uint16_t v[8];
    get_st(st, v);
    if (v[0] < 1601 || v[0] > 30827 || v[1] < 1 || v[1] > 12 || v[3] < 1 || v[3] > days_in(v[0], v[1]) || v[4] > 23 || v[5] > 59
        || v[6] > 59 || v[7] > 999)
        return err(87);
    int64_t days = 0;
    for (int y = 1601; y < v[0]; y++) days += 365 + ((y % 4 == 0 && y % 100) || y % 400 == 0);
    for (int m = 1; m < v[1]; m++) days += days_in(v[0], m);
    days += v[3] - 1;
    uint64_t t = ((((uint64_t)days * 24 + v[4]) * 60 + v[5]) * 60 + v[6]) * 10000000ull + v[7] * 10000ull;
    wr32(ft, (uint32_t)t); wr32(ft + 4, (uint32_t)(t >> 32));
    return 1;
}

uint32_t CompareFileTime_c(uint32_t a, uint32_t b)
{
    uint64_t x = rd32(a) | (uint64_t)rd32(a + 4) << 32, y = rd32(b) | (uint64_t)rd32(b + 4) << 32;
    return x < y ? 0xFFFFFFFFu : x > y ? 1u : 0u;
}

/* The daylight rule in Windows' form: month, day of week, occurrence (5 = last), hour. */
static void rule(uint32_t at, time_t when, int to_dst)
{
    struct tm t;
    localtime_r(&when, &t);
    int occ = (t.tm_mday - 1) / 7 + 1;
    if (t.tm_mday + 7 > days_in(t.tm_year + 1900, t.tm_mon + 1)) occ = 5;
    /* the transition happens at the old offset's local time */
    time_t before = when - 1;
    struct tm b;
    localtime_r(&before, &b);
    uint16_t v[8] = {0, (uint16_t)(t.tm_mon + 1), (uint16_t)t.tm_wday, (uint16_t)occ, (uint16_t)((b.tm_hour + 1) % 24), 0, 0, 0};
    (void)to_dst;
    memcpy(G(at), v, 16);
}

static void put_wname(uint32_t at, const char *s)
{
    uint16_t w[32] = {0};
    for (int i = 0; s[i] && i < 31; i++) w[i] = (uint8_t)s[i];
    memcpy(G(at), w, 64);
}

uint32_t GetTimeZoneInformation_c(uint32_t tzi)
{
    memset(G(tzi), 0, 172);
    time_t now = time(NULL);
    struct tm lt;
    localtime_r(&now, &lt);
    int year = lt.tm_year + 1900;
    /* the standard offset: the smaller UTC offset seen this year */
    struct tm jan = {0, 0, 12, 1, 0, year - 1900, 0, 0, -1}, jul = {0, 0, 12, 1, 6, year - 1900, 0, 0, -1};
    time_t tj = mktime(&jan), tu = mktime(&jul);
    struct tm a, b;
    localtime_r(&tj, &a); localtime_r(&tu, &b);
    long std_off = a.tm_gmtoff < b.tm_gmtoff ? a.tm_gmtoff : b.tm_gmtoff, dst_off = a.tm_gmtoff < b.tm_gmtoff ? b.tm_gmtoff : a.tm_gmtoff;
    wr32(tzi, (uint32_t)(int32_t)(-std_off / 60));                  /* Bias: UTC = local + bias */
    put_wname(tzi + 4, a.tm_gmtoff == std_off ? a.tm_zone : b.tm_zone);
    put_wname(tzi + 88, a.tm_gmtoff == dst_off ? a.tm_zone : b.tm_zone);
    if (std_off == dst_off) return 0;                               /* TIME_ZONE_ID_UNKNOWN: no daylight saving */
    wr32(tzi + 84, 0);                                              /* StandardBias */
    wr32(tzi + 168, (uint32_t)(int32_t)(-(dst_off - std_off) / 60));   /* DaylightBias */
    /* find the two transitions this year, hour by hour */
    struct tm start = {0, 0, 0, 1, 0, year - 1900, 0, 0, -1};
    time_t t = mktime(&start), end = t + 366 * 86400;
    struct tm prev;
    localtime_r(&t, &prev);
    for (t += 3600; t < end; t += 3600) {
        struct tm cur;
        localtime_r(&t, &cur);
        if (cur.tm_gmtoff != prev.tm_gmtoff) {
            time_t lo = t - 3600, hi = t;                           /* narrow to the second */
            while (hi - lo > 1) { time_t mid = lo + (hi - lo) / 2; struct tm m; localtime_r(&mid, &m); if (m.tm_gmtoff == prev.tm_gmtoff) lo = mid; else hi = mid; }
            if (cur.tm_gmtoff == dst_off) rule(tzi + 152, hi, 1);   /* DaylightDate */
            else rule(tzi + 68, hi, 0);                             /* StandardDate */
        }
        prev = cur;
    }
    return lt.tm_isdst > 0 ? 2u : 1u;                               /* TIME_ZONE_ID_DAYLIGHT / STANDARD */
}

/* ---- the locale ---- */

static const char *const days[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
static const char *const months[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September",
                                       "October", "November", "December"};

static int us(uint32_t lcid) { return lcid == LCID_US || lcid == 0x400 || lcid == 0x800 || lcid == 0; }
static void need_us(const char *what, uint32_t lcid) { if (!us(lcid)) hp_unsupported(what, "locale 0x%x (HaloPad presents English, US)", lcid); }

static const char *locale_value(uint32_t type, char *tmp)
{
    if (type >= 0x2A && type <= 0x30) return days[(type - 0x2A + 1) % 7];                       /* SDAYNAME1 = Monday */
    if (type >= 0x31 && type <= 0x37) { snprintf(tmp, 8, "%.3s", days[(type - 0x31 + 1) % 7]); return tmp; }
    if (type >= 0x38 && type <= 0x43) return months[type - 0x38];
    if (type >= 0x44 && type <= 0x4F) { snprintf(tmp, 8, "%.3s", months[type - 0x44]); return tmp; }
    switch (type) {
    case 0x01: return "0409"; case 0x02: return "English (United States)"; case 0x03: return "ENU"; case 0x04: return "English";
    case 0x05: return "1"; case 0x06: return "United States"; case 0x07: return "USA"; case 0x08: return "United States";
    case 0x09: return "0409"; case 0x0A: return "1"; case 0x0B: return "437"; case 0x0C: return ","; case 0x0D: return "1";
    case 0x0E: return "."; case 0x0F: return ","; case 0x10: return "3;0"; case 0x11: return "2"; case 0x12: return "1";
    case 0x13: return "0123456789"; case 0x14: return "$"; case 0x15: return "USD"; case 0x16: return "."; case 0x17: return ",";
    case 0x18: return "3;0"; case 0x19: return "2"; case 0x1A: return "2"; case 0x1B: return "0"; case 0x1C: return "0";
    case 0x1D: return "/"; case 0x1E: return ":"; case 0x1F: return "M/d/yyyy"; case 0x20: return "dddd, MMMM dd, yyyy";
    case 0x21: return "0"; case 0x22: return "0"; case 0x23: return "0"; case 0x24: return "1"; case 0x25: return "0";
    case 0x26: return "0"; case 0x27: return "0"; case 0x28: return "AM"; case 0x29: return "PM";
    case 0x50: return ""; case 0x51: return "-"; case 0x52: return "3"; case 0x53: return "0"; case 0x54: return "1";
    case 0x55: return "0"; case 0x56: return "1"; case 0x57: return "0"; case 0x59: return "en"; case 0x5A: return "US";
    case 0x1001: return "English"; case 0x1002: return "United States"; case 0x1003: return "h:mm:ss tt"; case 0x1004: return "1252";
    case 0x1006: return "MMMM, yyyy"; case 0x1007: return "US Dollar"; case 0x1008: return "US Dollar"; case 0x1009: return "1";
    case 0x100A: return "1"; case 0x100B: return "0"; case 0x100C: return "6"; case 0x100D: return "0"; case 0x1011: return "10000";
    case 0x1012: return "037";
    }
    return NULL;
}

static uint32_t locale_info(uint32_t lcid, uint32_t type, uint32_t buf, uint32_t n, int wide)
{
    need_us("GetLocaleInfo", lcid);
    char tmp[16];
    uint32_t base = type & 0x0FFFFFFFu;
    const char *v = locale_value(base, tmp);
    if (!v) hp_unsupported("GetLocaleInfo", "LCTYPE 0x%x", type);
    if (type & 0x20000000u) {                                       /* LOCALE_RETURN_NUMBER */
        uint32_t need = wide ? 2 : 4;                               /* a DWORD, counted in characters */
        if (n && n < need) return err(122);
        uint32_t num = (uint32_t)strtoul(v, NULL, base == 1 || base == 9 ? 16 : 10);
        if (n) wr32(buf, num);
        return need;
    }
    uint32_t len = (uint32_t)strlen(v) + 1;
    if (!n) return len;
    if (n < len) return err(122);                                   /* ERROR_INSUFFICIENT_BUFFER */
    if (wide) for (uint32_t i = 0; i < len; i++) wr16(buf + 2 * i, (uint8_t)v[i]);
    else memcpy(G(buf), v, len);
    return len;
}
uint32_t GetLocaleInfoA_c(uint32_t lcid, uint32_t type, uint32_t buf, uint32_t n) { return locale_info(lcid, type, buf, n, 0); }
uint32_t GetLocaleInfoW_c(uint32_t lcid, uint32_t type, uint32_t buf, uint32_t n) { return locale_info(lcid, type, buf, n, 1); }

uint32_t GetUserDefaultLCID_c(void) { return LCID_US; }
uint32_t GetUserDefaultLangID_c(void) { return LCID_US & 0xFFFF; }   /* LANGIDFROMLCID: English (United States) */
uint32_t GetSystemDefaultLangID_c(void) { return LCID_US & 0xFFFF; }
static _Thread_local uint32_t thread_locale = LCID_US;
uint32_t GetThreadLocale_c(void) { return thread_locale; }
uint32_t SetThreadLocale_c(uint32_t lcid) { need_us("SetThreadLocale", lcid); thread_locale = LCID_US; return 1; }
uint32_t IsValidLocale_c(uint32_t lcid, uint32_t flags) { (void)flags; return lcid == LCID_US; }

uint32_t EnumSystemLocalesA_c(uint32_t proc, uint32_t flags)
{
    if (flags != 1 && flags != 2) return err(1004);                 /* LCID_INSTALLED, LCID_SUPPORTED */
    uint32_t s = halopad_heap_alloc(9, 0);
    strcpy(G(s), "00000409");
    halopad_call_guest(proc, 1, &s);
    halopad_heap_free(s);
    return 1;
}

/* Date and time pictures (d, dd, ddd, dddd, M.., y.., h, hh, H, HH, m, mm, s, ss, t, tt, '...') */
static uint32_t format_picture(const char *pic, const uint16_t v[8], uint32_t buf, uint32_t n, uint32_t flags_time)
{
    char out[256];
    size_t o = 0;
    for (const char *p = pic; *p && o < sizeof out - 32;) {
        char c = *p;
        if (c == '\'') {
            p++;
            while (*p && *p != '\'') out[o++] = *p++;
            if (*p) p++;
            continue;
        }
        int k = 0;
        while (p[k] == c) k++;
        p += k;
        int h12 = v[4] % 12 ? v[4] % 12 : 12;
        switch (c) {
        case 'd':
            if (k <= 2) o += (size_t)snprintf(out + o, 8, k == 1 ? "%d" : "%02d", v[3]);
            else o += (size_t)snprintf(out + o, 16, k == 3 ? "%.3s" : "%s", days[v[2] % 7]);
            break;
        case 'M':
            if (k <= 2) o += (size_t)snprintf(out + o, 8, k == 1 ? "%d" : "%02d", v[1]);
            else o += (size_t)snprintf(out + o, 16, k == 3 ? "%.3s" : "%s", months[(v[1] + 11) % 12]);
            break;
        case 'y':
            if (k <= 2) o += (size_t)snprintf(out + o, 8, k == 1 ? "%d" : "%02d", v[0] % 100);
            else o += (size_t)snprintf(out + o, 8, "%d", v[0]);
            break;
        case 'g': o += (size_t)snprintf(out + o, 8, "A.D."); break;
        case 'h': o += (size_t)snprintf(out + o, 8, k == 1 ? "%d" : "%02d", (flags_time & 8) ? v[4] : h12); break;
        case 'H': o += (size_t)snprintf(out + o, 8, k == 1 ? "%d" : "%02d", v[4]); break;
        case 'm': o += (size_t)snprintf(out + o, 8, k == 1 ? "%d" : "%02d", v[5]); break;
        case 's': o += (size_t)snprintf(out + o, 8, k == 1 ? "%d" : "%02d", v[6]); break;
        case 't':
            if (!(flags_time & 4)) o += (size_t)snprintf(out + o, 8, k == 1 ? "%.1s" : "%s", v[4] < 12 ? "AM" : "PM");
            break;
        default:
            for (int i = 0; i < k; i++) out[o++] = c;
        }
    }
    out[o] = 0;
    /* trim a separator left dangling by a dropped field (TIME_NOTIMEMARKER, ...) */
    while (o && (out[o - 1] == ' ' || out[o - 1] == ':')) out[--o] = 0;
    uint32_t len = (uint32_t)o + 1;
    if (!n) return len;
    if (n < len) return err(122);
    memcpy(G(buf), out, len);
    return len;
}

static void now_local(uint16_t v[8])
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm t;
    localtime_r(&ts.tv_sec, &t);
    uint16_t x[8] = {(uint16_t)(t.tm_year + 1900), (uint16_t)(t.tm_mon + 1), (uint16_t)t.tm_wday, (uint16_t)t.tm_mday, (uint16_t)t.tm_hour,
                     (uint16_t)t.tm_min, (uint16_t)t.tm_sec, (uint16_t)(ts.tv_nsec / 1000000)};
    memcpy(v, x, 16);
}

uint32_t GetDateFormatA_c(uint32_t lcid, uint32_t flags, uint32_t st, uint32_t fmt, uint32_t buf, uint32_t n)
{
    need_us("GetDateFormatA", lcid);
    if (flags & ~(0x1u | 0x2u | 0x80000000u)) hp_unsupported("GetDateFormatA", "flags 0x%x", flags);
    if (fmt && (flags & 3)) return err(1004);                       /* ERROR_INVALID_FLAGS */
    uint16_t v[8];
    if (st) {
        get_st(st, v);
        if (v[1] < 1 || v[1] > 12 || v[3] < 1 || v[3] > days_in(v[0], v[1])) return err(87);
        struct tm t = {0, 0, 12, v[3], v[1] - 1, v[0] - 1900, 0, 0, -1};
        timegm(&t);
        v[2] = (uint16_t)t.tm_wday;                                 /* Windows ignores the given day of week */
    } else now_local(v);
    return format_picture(fmt ? (const char *)G(fmt) : (flags & 2) ? "dddd, MMMM dd, yyyy" : "M/d/yyyy", v, buf, n, 0);
}

uint32_t GetTimeFormatA_c(uint32_t lcid, uint32_t flags, uint32_t st, uint32_t fmt, uint32_t buf, uint32_t n)
{
    need_us("GetTimeFormatA", lcid);
    if (flags & ~(0x1u | 0x2u | 0x4u | 0x8u | 0x80000000u)) hp_unsupported("GetTimeFormatA", "flags 0x%x", flags);
    uint16_t v[8];
    if (st) {
        get_st(st, v);
        if (v[4] > 23 || v[5] > 59 || v[6] > 59) return err(87);
    } else now_local(v);
    char pic[64];
    snprintf(pic, sizeof pic, "%s", fmt ? (const char *)G(fmt) : ((flags & 8) ? "HH:mm:ss tt" : "h:mm:ss tt"));
    if (flags & 1) { char *m = strstr(pic, ":mm"); if (m) memmove(m, m + 3, strlen(m + 3) + 1); }   /* TIME_NOMINUTESORSECONDS */
    if (flags & 3) { char *s = strstr(pic, ":ss"); if (s) memmove(s, s + 3, strlen(s + 3) + 1); }   /* TIME_NOSECONDS */
    return format_picture(pic, v, buf, n, flags);
}

/* Windows' word sort, approximated for the characters Halo's strings use: letters compare
   without case first, then lowercase before uppercase; punctuation sorts before digits,
   digits before letters; '-' and '\'' are ignored unless SORT_STRINGSORT. */
static int weight(unsigned c)
{
    if (isalpha(c)) return 0x300 + tolower(c);
    if (isdigit(c)) return 0x200 + c;
    return 0x100 + c;
}
static uint32_t compare(uint32_t flags, const uint8_t *a, int na, const uint8_t *b, int nb)
{
    int ic = flags & 1, word = !(flags & 0x1000);
    int i = 0, j = 0, tie = 0;
    for (;;) {
        while (word && i < na && (a[i] == '-' || a[i] == '\'')) i++;
        while (word && j < nb && (b[j] == '-' || b[j] == '\'')) j++;
        if (i >= na || j >= nb) break;
        int wa = weight(a[i]), wb = weight(b[j]);
        if (wa != wb) return wa < wb ? 1u : 3u;
        if (!ic && !tie && a[i] != b[j]) tie = islower(a[i]) ? 1 : 3;
        i++; j++;
    }
    if ((i < na) != (j < nb)) return i < na ? 3u : 1u;
    if (tie) return (uint32_t)tie;
    if (word && na != nb) return na < nb ? 1u : 3u;                 /* then the ignored characters count */
    return 2u;
}
static int slen(uint32_t s, uint32_t n, int wide)
{
    if ((int32_t)n >= 0) return (int)n;
    int k = 0;
    if (wide) while (((uint16_t *)G(s))[k]) k++;
    else while (((uint8_t *)G(s))[k]) k++;
    return k;
}
uint32_t CompareStringA_c(uint32_t lcid, uint32_t flags, uint32_t s1, uint32_t n1, uint32_t s2, uint32_t n2)
{
    need_us("CompareStringA", lcid);
    if (flags & ~(0x1u | 0x2u | 0x4u | 0x10000u | 0x20000u | 0x1000u)) hp_unsupported("CompareStringA", "flags 0x%x", flags);
    if (!s1 || !s2) return err(87);
    return compare(flags, G(s1), slen(s1, n1, 0), G(s2), slen(s2, n2, 0));
}
uint32_t CompareStringW_c(uint32_t lcid, uint32_t flags, uint32_t s1, uint32_t n1, uint32_t s2, uint32_t n2)
{
    need_us("CompareStringW", lcid);
    if (flags & ~(0x1u | 0x2u | 0x4u | 0x10000u | 0x20000u | 0x1000u)) hp_unsupported("CompareStringW", "flags 0x%x", flags);
    if (!s1 || !s2) return err(87);
    int na = slen(s1, n1, 1), nb = slen(s2, n2, 1);
    uint8_t *a = malloc((size_t)na + 1), *b = malloc((size_t)nb + 1);
    for (int i = 0; i < na; i++) { uint16_t c = ((uint16_t *)G(s1))[i]; if (c > 0xFF) hp_unsupported("CompareStringW", "character U+%04X", c); a[i] = (uint8_t)c; }
    for (int i = 0; i < nb; i++) { uint16_t c = ((uint16_t *)G(s2))[i]; if (c > 0xFF) hp_unsupported("CompareStringW", "character U+%04X", c); b[i] = (uint8_t)c; }
    uint32_t r = compare(flags, a, na, b, nb);
    free(a); free(b);
    return r;
}

/* ---- FormatMessageA from the system table ---- */

static const struct { uint32_t id; const char *text; } messages[] = {
    {0, "The operation completed successfully.\r\n"}, {1, "Incorrect function.\r\n"},
    {2, "The system cannot find the file specified.\r\n"}, {3, "The system cannot find the path specified.\r\n"},
    {4, "The system cannot open the file.\r\n"}, {5, "Access is denied.\r\n"}, {6, "The handle is invalid.\r\n"},
    {8, "Not enough storage is available to process this command.\r\n"}, {14, "Not enough storage is available to complete this operation.\r\n"},
    {18, "There are no more files.\r\n"}, {32, "The process cannot access the file because it is being used by another process.\r\n"},
    {38, "Reached the end of the file.\r\n"}, {80, "The file exists.\r\n"}, {87, "The parameter is incorrect.\r\n"},
    {112, "There is not enough space on the disk.\r\n"}, {122, "The data area passed to a system call is too small.\r\n"},
    {123, "The filename, directory name, or volume label syntax is incorrect.\r\n"}, {126, "The specified module could not be found.\r\n"},
    {127, "The specified procedure could not be found.\r\n"}, {131, "An attempt was made to move the file pointer before the beginning of the file.\r\n"},
    {145, "The directory is not empty.\r\n"}, {183, "Cannot create a file when that file already exists.\r\n"},
    {259, "No more data is available.\r\n"}, {267, "The directory name is invalid.\r\n"}, {997, "Overlapped I/O operation is in progress.\r\n"},
    {1400, "Invalid window handle.\r\n"}, {1407, "Cannot find window class.\r\n"}, {1410, "Class already exists.\r\n"},
    {1411, "Class does not exist.\r\n"},
    {10035, "A non-blocking socket operation could not be completed immediately.\r\n"},
    {10040, "A message sent on a datagram socket was larger than the internal message buffer or some other network limit, or the buffer used to receive a datagram into was smaller than the datagram itself.\r\n"},
    {10048, "Only one usage of each socket address (protocol/network address/port) is normally permitted.\r\n"},
    {10054, "An existing connection was forcibly closed by the remote host.\r\n"},
    {10060, "A connection attempt failed because the connected party did not properly respond after a period of time, or established connection failed because connected host has failed to respond.\r\n"},
    {10061, "No connection could be made because the target machine actively refused it.\r\n"},
    {11001, "No such host is known.\r\n"},
};

/* FormatMessageA/W, as on Windows XP. The text comes from a module's own message table
 * (FORMAT_MESSAGE_FROM_HMODULE; a NULL module is the executable, which has none), then the
 * system table (FROM_SYSTEM; an HRESULT_FROM_WIN32 value, 0x8007xxxx, gives the Win32
 * error's message). msxml4.dll takes its messages from msxml4r.dll's table this way. Unless
 * IGNORE_INSERTS: %1..%99 inserts with an optional !printf format! (default !s!: a string
 * of the caller's kind, narrow for A and wide for W), from an argument array
 * (ARGUMENT_ARRAY) or a va_list; escapes %0 %n %% %. %! %<space> %t %r. MAX_WIDTH 0xFF
 * drops the text's own line breaks. The work is done in UTF-16. */
uint32_t FindResourceExA_c(uint32_t module, uint32_t type, uint32_t name, uint32_t lang);
uint32_t LoadResource_c(uint32_t module, uint32_t hrsrc);
uint32_t halopad_heap_alloc(uint32_t size, int zero);
void halopad_heap_free(uint32_t p);
static uint16_t rd16g(uint32_t a) { uint16_t v; memcpy(&v, G(a), 2); return v; }

/* message text as UTF-16 into t (NUL-terminated); 0 or a Win32 error */
static uint32_t fm_text(uint32_t flags, uint32_t source, uint32_t id, uint32_t lang, uint16_t *t, size_t cap)
{
    if (lang && (lang & 0x3FF) != 0x09 && lang != 0x400 && lang != 0x800) return 15100;   /* ERROR_RESOURCE_LANG_NOT_FOUND */
    uint32_t module_error = 0;
    if (flags & 0x800u) {                                            /* FROM_HMODULE */
        uint32_t res = FindResourceExA_c(source ? source : HP_IMAGE_BASE, 11 /* RT_MESSAGETABLE */, 1, 0);
        if (!res) module_error = 1813;                               /* ERROR_RESOURCE_TYPE_NOT_FOUND */
        else {
            uint32_t d = LoadResource_c(source ? source : HP_IMAGE_BASE, res);
            uint32_t blocks = rd32(d);
            module_error = 317;
            for (uint32_t b = 0; b < blocks; b++) {
                uint32_t lo = rd32(d + 4 + 12 * b), hi = rd32(d + 8 + 12 * b), off = rd32(d + 12 + 12 * b);
                if (id < lo || id > hi) continue;
                uint32_t e = d + off;
                for (uint32_t k = lo; k < id; k++) e += rd16g(e);
                uint32_t n = rd16g(e) - 4, uni = rd16g(e + 2) & 1;
                size_t o = 0;
                if (uni) for (uint32_t i = 0; i + 1 < n && o + 1 < cap; i += 2) { uint16_t c = rd16g(e + 4 + i); if (!c) break; t[o++] = c; }
                else for (uint32_t i = 0; i < n && o + 1 < cap; i++) { uint8_t c = *(uint8_t *)G(e + 4 + i); if (!c) break; t[o++] = c; }
                t[o] = 0;
                return 0;
            }
        }
    }
    if (flags & 0x1000u) {                                           /* FROM_SYSTEM */
        const char *text = NULL;
        for (size_t i = 0; i < sizeof messages / sizeof messages[0]; i++) if (messages[i].id == id) text = messages[i].text;
        if (!text && (id >> 16) == 0x8007u)
            for (size_t i = 0; i < sizeof messages / sizeof messages[0]; i++) if (messages[i].id == (id & 0xFFFFu)) text = messages[i].text;
        if (text) {
            size_t o = 0;
            for (; text[o] && o + 1 < cap; o++) t[o] = (uint8_t)text[o];
            t[o] = 0;
            return 0;
        }
        return 317;                                                  /* ERROR_MR_MID_NOT_FOUND */
    }
    return module_error ? module_error : 87;
}

static void fm_put(uint16_t *out, size_t *o, size_t cap, uint16_t c) { if (*o + 1 < cap) out[(*o)++] = c; }
static void fm_puts(uint16_t *out, size_t *o, size_t cap, const char *s) { while (*s) fm_put(out, o, cap, (uint8_t)*s++); }

/* formatted message as UTF-16; returns 0 or a Win32 error, *len the character count */
static uint32_t fm_core(uint32_t flags, uint32_t source, uint32_t id, uint32_t lang, uint32_t args, int wide,
                        uint16_t *out, size_t cap, size_t *len)
{
    if (flags & ~(0x100u | 0x200u | 0x800u | 0x1000u | 0x2000u | 0xFFu))
        hp_unsupported(wide ? "FormatMessageW" : "FormatMessageA", "flags 0x%x, source 0x%08x, message 0x%08x", flags, source, id);
    uint16_t t[2048];
    uint32_t e = fm_text(flags, source, id, lang, t, sizeof t / sizeof t[0]);
    if (e) return e;
    int inserts = !(flags & 0x200u), nobreaks = (flags & 0xFFu) == 0xFFu;
    if ((flags & 0xFFu) && !nobreaks) hp_unsupported(wide ? "FormatMessageW" : "FormatMessageA", "line width %u", flags & 0xFFu);
    size_t o = 0;
    for (const uint16_t *p = t; *p; p++) {
        if (inserts && *p == '%' && p[1]) {
            uint16_t c = *++p;
            if (c >= '1' && c <= '9') {
                int n = c - '0';
                if (p[1] >= '0' && p[1] <= '9') n = n * 10 + (*++p - '0');
                char spec[16] = "s";
                if (p[1] == '!') {                                   /* %n!format! */
                    size_t k = 0;
                    p += 2;
                    while (*p && *p != '!' && k + 1 < sizeof spec) spec[k++] = (char)*p++;
                    spec[k] = 0;
                    if (!*p) break;
                }
                if (!args) return 87;                                /* ERROR_INVALID_PARAMETER */
                uint32_t list = (flags & 0x2000u) ? args : rd32(args);   /* ARGUMENT_ARRAY, or a va_list */
                uint32_t v = rd32(list + 4 * (uint32_t)(n - 1));
                char last = spec[strlen(spec) - 1];
                if (last == 's' || last == 'S') {
                    int w = (last == 's') == (wide != 0);            /* !s! is the caller's kind, !S! the other */
                    if (spec[0] != 's' && spec[0] != 'S') hp_unsupported("FormatMessage", "string insert format !%s!", spec);
                    if (!v) { fm_puts(out, &o, cap, "(null)"); continue; }
                    if (w) for (uint32_t i = 0; rd16g(v + 2 * i); i++) fm_put(out, &o, cap, rd16g(v + 2 * i));
                    else for (const char *s = G(v); *s; s++) fm_put(out, &o, cap, (uint8_t)*s);
                } else if (strchr("diuxXoc", last)) {
                    char f[24], b[64];
                    snprintf(f, sizeof f, "%%%s", spec);
                    for (char *q = f; *q; q++) if (*q == 'l' || *q == 'h') memmove(q, q + 1, strlen(q));   /* 32-bit values */
                    snprintf(b, sizeof b, f, v);
                    fm_puts(out, &o, cap, b);
                } else hp_unsupported("FormatMessage", "insert format !%s!", spec);
                continue;
            }
            if (c == '0') break;                                     /* end, without a line break */
            if (c == 'n') { fm_put(out, &o, cap, '\r'); fm_put(out, &o, cap, '\n'); continue; }
            if (c == 't') { fm_put(out, &o, cap, '\t'); continue; }
            if (c == 'r') { fm_put(out, &o, cap, '\r'); continue; }
            fm_put(out, &o, cap, c);                                 /* %% %. %! %<space> */
            continue;
        }
        if (nobreaks && (*p == '\r' || *p == '\n')) { if (*p == '\n') fm_put(out, &o, cap, ' '); continue; }
        fm_put(out, &o, cap, *p);
    }
    out[o] = 0;
    *len = o;
    return 0;
}

uint32_t FormatMessageA_c(uint32_t flags, uint32_t source, uint32_t id, uint32_t lang, uint32_t buf, uint32_t size, uint32_t args)
{
    uint16_t out[4096];
    size_t len;
    uint32_t e = fm_core(flags, source, id, lang, args, 0, out, sizeof out / sizeof out[0], &len);
    if (e) return err(e);
    uint32_t dst;
    if (flags & 0x100u) {                                            /* ALLOCATE_BUFFER: LocalAlloc, pointer stored at buf */
        dst = LocalAlloc_c(0, (uint32_t)len + 1 > size ? (uint32_t)len + 1 : size);
        wr32(buf, dst);
    } else {
        if (size < len + 1) return err(122);
        dst = buf;
    }
    char *d = G(dst);
    for (size_t i = 0; i < len; i++) d[i] = out[i] < 0x100 ? (char)out[i] : '?';
    d[len] = 0;
    return (uint32_t)len;
}

uint32_t FormatMessageW_c(uint32_t flags, uint32_t source, uint32_t id, uint32_t lang, uint32_t buf, uint32_t size, uint32_t args)
{
    uint16_t out[4096];
    size_t len;
    uint32_t e = fm_core(flags, source, id, lang, args, 1, out, sizeof out / sizeof out[0], &len);
    if (e) return err(e);
    uint32_t dst;
    if (flags & 0x100u) {
        dst = LocalAlloc_c(0, 2 * ((uint32_t)len + 1 > size ? (uint32_t)len + 1 : size));
        wr32(buf, dst);
    } else {
        if (size < len + 1) return err(122);
        dst = buf;
    }
    memcpy(G(dst), out, 2 * (len + 1));
    return (uint32_t)len;
}


/* ---- memory, pointers, process ---- */

uint32_t GlobalMemoryStatus_c(uint32_t ms)
{
    uint64_t phys = 0;
    size_t l = sizeof phys;
    sysctlbyname("hw.memsize", &phys, &l, NULL, 0);
    uint32_t total = phys > 0x7FFFFFFFull ? 0x7FFFFFFFu : (uint32_t)phys;   /* as a large-address-unaware program sees it */
    wr32(ms, 32); wr32(ms + 4, 30);
    wr32(ms + 8, total); wr32(ms + 12, total / 2);
    wr32(ms + 16, 0x7FFFFFFFu); wr32(ms + 20, 0x6FFFFFFFu);
    wr32(ms + 24, 0x7FFE0000u); wr32(ms + 28, 0x60000000u);
    return 0;
}

static int accessible(uint32_t p, uint32_t n, int write)
{
    if (!n) return 1;
    uint32_t mbi = halopad_heap_alloc(28, 1), a = p & ~0xFFFu, end = p + n;
    if (end < p) { halopad_heap_free(mbi); return 0; }
    int ok = 1;
    while (a < end && ok) {
        if (!VirtualQuery_c(a, mbi, 28)) { ok = 0; break; }
        uint32_t state = rd32(mbi + 16), prot = rd32(mbi + 20), rsize = rd32(mbi + 12);
        if (state != 0x1000 || (prot & 0x1) || (prot & 0x100) || (write && !(prot & 0x44))) ok = 0;   /* MEM_COMMIT; not NOACCESS/GUARD */
        a = rd32(mbi) + (rsize ? rsize : 0x1000);
    }
    halopad_heap_free(mbi);
    return ok;
}
uint32_t IsBadReadPtr_c(uint32_t p, uint32_t n) { return !accessible(p, n, 0); }
uint32_t IsBadWritePtr_c(uint32_t p, uint32_t n) { return !accessible(p, n, 1); }
uint32_t IsBadCodePtr_c(uint32_t p) { return !accessible(p, 1, 0); }

static uint32_t error_mode, priority_class = 0x20;
uint32_t SetErrorMode_c(uint32_t mode) { uint32_t old = error_mode; error_mode = mode; return old; }
uint32_t GetPriorityClass_c(uint32_t process) { if (process != 0xFFFFFFFFu) return err(6); return priority_class; }
uint32_t SetPriorityClass_c(uint32_t process, uint32_t cls)
{
    if (process != 0xFFFFFFFFu) return err(6);
    if (cls != 0x20 && cls != 0x40 && cls != 0x80 && cls != 0x100 && cls != 0x4000 && cls != 0x8000) return err(87);
    priority_class = cls;                                           /* recorded; the host schedules the process normally */
    return 1;
}

uint32_t TerminateProcess_c(uint32_t process, uint32_t code)
{
    if (process != 0xFFFFFFFFu) hp_unsupported("TerminateProcess", "another process (0x%x)", process);
    fprintf(stderr, "HALOPAD: TerminateProcess(%u)\n", code);
    fflush(stdout); fflush(stderr);
    _exit((int)code);                                               /* no DLL detach or atexit, as on Windows */
}
