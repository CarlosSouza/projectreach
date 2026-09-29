/* HaloPad's always-on diagnostic log (port/runtime/halopad_log.c). */
#ifndef HALOPAD_LOG_H
#define HALOPAD_LOG_H
void halopad_log_open(const char *path);
const char *halopad_log_path(void);
__attribute__((weak, format(printf, 1, 2))) void halopad_log(const char *fmt, ...);
/* Runtime call sites: harnesses that link single runtime files have no log. */
#define HP_LOG(...) do { if (halopad_log) halopad_log(__VA_ARGS__); } while (0)
#endif
