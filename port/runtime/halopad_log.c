/* HaloPad's always-on diagnostic log.

   One line per change a player can report: controllers connecting, Halo's
   controller devices being acquired, lost or reassigned, menu/game state, video
   mode resets and long frame stalls. Callers log transitions only, so the file
   stays small. It never records typed text, names, chat or server addresses.
   On iOS the app opens Documents/HaloPad Logs/HaloPad.log (visible in Files);
   every line also goes to stderr for a device console. */
#include "halopad_log.h"
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>

static FILE *log_file;
static char log_file_path[1024];
static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;

void halopad_log_open(const char *path)
{
    pthread_mutex_lock(&log_lock);
    if (!log_file && path && strlen(path) < sizeof log_file_path - 8) {
        struct stat st;
        if (!stat(path, &st) && st.st_size > (1 << 20)) {     /* keep one previous 1 MB file */
            char old[sizeof log_file_path];
            snprintf(old, sizeof old, "%s.old", path);
            rename(path, old);
        }
        log_file = fopen(path, "a");
        if (log_file) { setvbuf(log_file, NULL, _IOLBF, 0); strcpy(log_file_path, path); }
    }
    pthread_mutex_unlock(&log_lock);
}

const char *halopad_log_path(void) { return log_file ? log_file_path : NULL; }

void halopad_log(const char *fmt, ...)
{
    char text[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(text, sizeof text, fmt, ap);
    va_end(ap);
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm tm;
    localtime_r(&tv.tv_sec, &tm);
    char stamp[32];
    snprintf(stamp, sizeof stamp, "%04d-%02d-%02d %02d:%02d:%02d.%03d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec, (int)(tv.tv_usec / 1000));
    pthread_mutex_lock(&log_lock);
    fprintf(stderr, "HALOPAD LOG %s %s\n", stamp, text);
    if (log_file) fprintf(log_file, "%s %s\n", stamp, text);
    pthread_mutex_unlock(&log_lock);
}
