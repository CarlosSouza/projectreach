/*
 * xg_darwin_compat.h: lets upstream's port/linux/src/posix_*.c (written for
 * glibc) compile on Darwin. Forced into those files by the build script.
 */
#ifndef XG_DARWIN_COMPAT_H
#define XG_DARWIN_COMPAT_H
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#define st_mtim st_mtimespec
#define st_atim st_atimespec
#define st_ctim st_ctimespec
#ifndef SOCK_CLOEXEC
#define SOCK_CLOEXEC 0
#endif
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
static inline int accept4(int s, struct sockaddr *a, socklen_t *l, int flags) { (void)flags; return accept(s, a, l); }
static inline ssize_t getrandom(void *buffer, size_t size, unsigned flags) { (void)flags; arc4random_buf(buffer, size); return (ssize_t)size; }
#endif
