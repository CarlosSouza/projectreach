/* HaloPad Winsock (G5/G6): WS2_32 and WSOCK32 on the host's BSD sockets.
 *
 * How Halo uses it (haloce.exe): both DLLs are delay-loaded; the calls come from its
 * GameSpy networking library (0x5b9000-0x5c7000): UDP sockets (socket(2, 2, 0|17)) for game
 * traffic and server queries (15 sendto sites, recvfrom), TCP (socket(2, 1, 6)) with
 * connect/send/recv/shutdown for the master server list, SOL_SOCKET options including
 * SO_RCVBUF, non-blocking sockets through ioctlsocket(FIONBIO), select loops, and
 * gethostname/gethostbyname/inet_addr/inet_ntoa for addresses.
 *
 * SOCKET values are handles (0x2000 + 4*i) mapped to host descriptors. Addresses are
 * converted between Winsock's sockaddr_in (16-bit family) and the host's (length +
 * 8-bit family); fd_sets are Winsock's counted arrays, served with poll(). Errors are
 * Winsock's (WSAE*), per thread, and also the thread's last error, as in Winsock.
 * Winsock behaviours kept: a datagram longer than the buffer fills it and fails with
 * WSAEMSGSIZE; a non-blocking connect reports WSAEWOULDBLOCK; a failed connect appears
 * in select's exception set. Options, ioctls and address families outside this set stop
 * with their values.
 *
 * Network policy (HALOPAD_NET): "lan" models the reference machine on a local network with
 * no internet. Only loopback, private (RFC 1918), link-local and broadcast destinations are
 * reachable (others fail with WSAENETUNREACH), and only the machine's own name and
 * "localhost" resolve (others fail with WSAHOST_NOT_FOUND, as offline). scripts/run-core.py
 * sets it for test runs unless it is set; scripts/public-join.sh sets "internet" to join
 * public servers. Unset or any other value: the host's network as it is. */
#include "halopad_win32.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <strings.h>
#include <unistd.h>

#define SOCK_BASE 0x2000u
#define MAXSOCK 256
#define INVALID_SOCKET 0xFFFFFFFFu
#define SOCKET_ERROR 0xFFFFFFFFu

typedef struct { int fd, used, nonblocking, connecting, type; } sock;
static sock socks[MAXSOCK];
static pthread_mutex_t slock = PTHREAD_MUTEX_INITIALIZER;
static int started;
static _Thread_local uint32_t wsa_error;
static _Thread_local uint32_t ntoa_buf, hostent_buf;

static uint32_t fail(uint32_t e) { wsa_error = e; halopad_last_error = e; return SOCKET_ERROR; }

static uint32_t wsa_of_errno(int e)
{
    switch (e) {
    case EWOULDBLOCK: case EINPROGRESS: return 10035;               /* WSAEWOULDBLOCK (also a pending connect) */
    case EALREADY: return 10037;
    case ENOTSOCK: return 10038;
    case EDESTADDRREQ: return 10039;
    case EMSGSIZE: return 10040;
    case EPROTOTYPE: return 10041;
    case ENOPROTOOPT: return 10042;
    case EPROTONOSUPPORT: return 10043;
    case EOPNOTSUPP: return 10045;
    case EAFNOSUPPORT: return 10047;
    case EADDRINUSE: return 10048;
    case EADDRNOTAVAIL: return 10049;
    case ENETDOWN: return 10050;
    case ENETUNREACH: return 10051;
    case ECONNABORTED: return 10053;
    case ECONNRESET: case EPIPE: return 10054;
    case ENOBUFS: return 10055;
    case EISCONN: return 10056;
    case ENOTCONN: return 10057;
    case ETIMEDOUT: return 10060;
    case ECONNREFUSED: return 10061;
    case EHOSTUNREACH: return 10065;
    case EACCES: return 10013;
    case EINVAL: return 10022;
    case EMFILE: return 10024;
    case EINTR: return 10004;
    }
    return 10022;
}
static uint32_t fail_errno(void) { return fail(wsa_of_errno(errno)); }

static sock *S(uint32_t s)
{
    if (s < SOCK_BASE || (s - SOCK_BASE) % 4 || (s - SOCK_BASE) / 4 >= MAXSOCK) return NULL;
    sock *k = &socks[(s - SOCK_BASE) / 4];
    return k->used ? k : NULL;
}
#define NEED_STARTED() do { if (!started) return fail(10093); } while (0)          /* WSANOTINITIALISED */
#define NEED_SOCK(k, s) sock *k = S(s); if (!k) return fail(10038)                /* WSAENOTSOCK */

/* Winsock sockaddr_in <-> host */
static int to_host(uint32_t name, uint32_t len, struct sockaddr_in *out)
{
    if (!name || len < 16) return 0;
    uint16_t fam = (uint16_t)rd32(name);
    if (fam != 2) return -1;
    memset(out, 0, sizeof *out);
    out->sin_len = sizeof *out;
    out->sin_family = AF_INET;
    memcpy(&out->sin_port, (uint8_t *)G(name) + 2, 2);
    memcpy(&out->sin_addr, (uint8_t *)G(name) + 4, 4);
    return 1;
}
static void to_guest(const struct sockaddr_in *a, uint32_t name, uint32_t lenp)
{
    if (!name) return;
    uint8_t w[16] = {2, 0};
    memcpy(w + 2, &a->sin_port, 2);
    memcpy(w + 4, &a->sin_addr, 4);
    uint32_t n = lenp ? rd32(lenp) : 16;
    memcpy(G(name), w, n < 16 ? n : 16);
    if (lenp) wr32(lenp, 16);
}

/* ---- start-up ---- */

uint32_t WSAStartup_c(uint32_t version, uint32_t data)
{
    uint8_t major = version & 0xFF, minor = (version >> 8) & 0xFF;
    if (major < 1 || (major == 1 && minor < 1)) return 10092;       /* WSAVERNOTSUPPORTED */
    if (!data) return 10014;                                        /* WSAEFAULT */
    uint16_t v = major > 2 || (major == 2 && minor >= 2) ? 0x0202 : (uint16_t)version;
    memset(G(data), 0, 400);
    wr16(data, v); wr16(data + 2, 0x0202);
    strcpy((char *)G(data + 4), "WinSock 2.0");
    strcpy((char *)G(data + 261), "Running");
    wr16(data + 390, 0); wr16(data + 392, 0);                       /* iMaxSockets, iMaxUdpDg: 0 for version 2 */
    pthread_mutex_lock(&slock);
    started++;
    pthread_mutex_unlock(&slock);
    return 0;
}
uint32_t WSACleanup_c(void)
{
    NEED_STARTED();
    pthread_mutex_lock(&slock);
    started--;
    pthread_mutex_unlock(&slock);
    return 0;
}
uint32_t WSAGetLastError_c(void) { return wsa_error; }

/* ---- sockets ---- */

uint32_t socket_c(uint32_t af, uint32_t type, uint32_t proto)
{
    if (!started) { fail(10093); return INVALID_SOCKET; }
    if (af != 2) { fail(10047); return INVALID_SOCKET; }            /* WSAEAFNOSUPPORT */
    if (type != 1 && type != 2) hp_unsupported("socket", "type %u", type);
    if (proto && proto != 6 && proto != 17) hp_unsupported("socket", "protocol %u", proto);
    int fd = socket(AF_INET, type == 1 ? SOCK_STREAM : SOCK_DGRAM, (int)proto);
    if (fd < 0) { fail_errno(); return INVALID_SOCKET; }
    int one = 1;
    setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof one);   /* Winsock reports a broken pipe as an error */
    pthread_mutex_lock(&slock);
    for (uint32_t i = 0; i < MAXSOCK; i++)
        if (!socks[i].used) {
            socks[i] = (sock){fd, 1, 0, 0, (int)type};
            pthread_mutex_unlock(&slock);
            return SOCK_BASE + 4 * i;
        }
    pthread_mutex_unlock(&slock);
    close(fd);
    fail(10024);                                                    /* WSAEMFILE */
    return INVALID_SOCKET;
}

uint32_t closesocket_c(uint32_t s)
{
    NEED_STARTED();
    pthread_mutex_lock(&slock);
    sock *k = S(s);
    if (!k) { pthread_mutex_unlock(&slock); return fail(10038); }
    int fd = k->fd;
    k->used = 0;
    pthread_mutex_unlock(&slock);
    close(fd);
    return 0;
}

static int lan_only(void)
{
    static int v = -1;
    if (v < 0) { const char *e = getenv("HALOPAD_NET"); v = e && !strcmp(e, "lan"); }
    return v;
}

/* Diagnostics: HALOPAD_TRACE_NET=1 prints binds, datagrams and connects (address, port, size,
   the first bytes). */
static int trace_net(void)
{
    static int v = -1;
    if (v < 0) v = getenv("HALOPAD_TRACE_NET") != NULL;
    return v;
}
static void trace(const char *what, uint32_t s, const struct sockaddr_in *a, const void *data, long n)
{
    if (!trace_net()) return;
    char ip[32];
    inet_ntop(AF_INET, &a->sin_addr, ip, sizeof ip);
    fprintf(stderr, "HALOPAD NET %s socket 0x%x %s:%u %ld bytes:", what, s, ip, ntohs(a->sin_port), n);
    for (long i = 0; data && i < n && i < 64; i++) fprintf(stderr, " %02x", ((const uint8_t *)data)[i]);
    fprintf(stderr, "\n");
}

/* Under the "lan" policy: loopback, 10/8, 172.16/12, 192.168/16, 169.254/16 and broadcast. */
static int reachable(const struct sockaddr_in *a)
{
    if (!lan_only()) return 1;
    uint32_t h = ntohl(a->sin_addr.s_addr);
    return (h >> 24) == 127 || (h >> 24) == 10 || (h >> 20) == 0xAC1 || (h >> 16) == 0xC0A8 || (h >> 16) == 0xA9FE
        || h == 0xFFFFFFFFu;
}

uint32_t bind_c(uint32_t s, uint32_t name, uint32_t len)
{
    NEED_STARTED(); NEED_SOCK(k, s);
    struct sockaddr_in a;
    int r = to_host(name, len, &a);
    if (r < 0) return fail(10047);
    if (!r) return fail(10014);
    trace("bind", s, &a, NULL, 0);
    if (bind(k->fd, (struct sockaddr *)&a, sizeof a)) return fail_errno();
    return 0;
}

uint32_t connect_c(uint32_t s, uint32_t name, uint32_t len)
{
    NEED_STARTED(); NEED_SOCK(k, s);
    struct sockaddr_in a;
    int r = to_host(name, len, &a);
    if (r < 0) return fail(10047);
    if (!r) return fail(10014);
    if (!reachable(&a)) return fail(10051);                         /* WSAENETUNREACH */
    if (connect(k->fd, (struct sockaddr *)&a, sizeof a)) {
        if (errno == EINPROGRESS) k->connecting = 1;
        return fail_errno();
    }
    return 0;
}

uint32_t getsockname_c(uint32_t s, uint32_t name, uint32_t lenp)
{
    NEED_STARTED(); NEED_SOCK(k, s);
    if (!name || !lenp || rd32(lenp) < 16) return fail(10014);
    struct sockaddr_in a;
    socklen_t n = sizeof a;
    if (getsockname(k->fd, (struct sockaddr *)&a, &n)) return fail_errno();
    to_guest(&a, name, lenp);
    return 0;
}

uint32_t shutdown_c(uint32_t s, uint32_t how)
{
    NEED_STARTED(); NEED_SOCK(k, s);
    if (how > 2) return fail(10022);
    if (shutdown(k->fd, (int)how)) return fail_errno();             /* SD_RECEIVE/SEND/BOTH = SHUT_RD/WR/RDWR */
    return 0;
}

static int flags_of(uint32_t f)
{
    if (f & ~0x3u) hp_unsupported("recv/send", "flags 0x%x", f);    /* MSG_OOB 1, MSG_PEEK 2 */
    return (f & 1 ? MSG_OOB : 0) | (f & 2 ? MSG_PEEK : 0);
}

uint32_t send_c(uint32_t s, uint32_t buf, uint32_t len, uint32_t flags)
{
    NEED_STARTED(); NEED_SOCK(k, s);
    ssize_t n = send(k->fd, len ? G(buf) : "", len, flags_of(flags));
    if (n < 0) return fail_errno();
    return (uint32_t)n;
}

uint32_t sendto_c(uint32_t s, uint32_t buf, uint32_t len, uint32_t flags, uint32_t to, uint32_t tolen)
{
    NEED_STARTED(); NEED_SOCK(k, s);
    struct sockaddr_in a;
    int r = to ? to_host(to, tolen, &a) : 0;
    if (r < 0) return fail(10047);
    if (to && !r) return fail(10014);
    if (r && !reachable(&a)) return fail(10051);                   /* WSAENETUNREACH */
    if (r) trace("sendto", s, &a, len ? G(buf) : NULL, len);
    ssize_t n = sendto(k->fd, len ? G(buf) : "", len, flags_of(flags), r ? (struct sockaddr *)&a : NULL, r ? sizeof a : 0);
    if (n < 0) return fail_errno();
    return (uint32_t)n;
}

static uint32_t receive(sock *k, uint32_t buf, uint32_t len, uint32_t flags, uint32_t from, uint32_t fromlen)
{
    struct sockaddr_in a;
    struct iovec v = {len ? G(buf) : NULL, len};
    struct msghdr m = {&a, sizeof a, &v, 1, NULL, 0, 0};
    if (from && (!fromlen || rd32(fromlen) < 16)) return fail(10014);
    ssize_t n = recvmsg(k->fd, &m, flags_of(flags));
    if (n < 0) return fail_errno();
    if (m.msg_namelen) trace("recvfrom", (uint32_t)(0x2000 + 4 * (k - socks)), &a, len ? G(buf) : NULL, n);
    if (from && m.msg_namelen) to_guest(&a, from, fromlen);
    if (k->type == 2 && (m.msg_flags & MSG_TRUNC)) return fail(10040);   /* WSAEMSGSIZE: the buffer holds the start */
    return (uint32_t)n;
}

uint32_t recv_c(uint32_t s, uint32_t buf, uint32_t len, uint32_t flags)
{
    NEED_STARTED(); NEED_SOCK(k, s);
    return receive(k, buf, len, flags, 0, 0);
}

uint32_t recvfrom_c(uint32_t s, uint32_t buf, uint32_t len, uint32_t flags, uint32_t from, uint32_t fromlen)
{
    NEED_STARTED(); NEED_SOCK(k, s);
    return receive(k, buf, len, flags, from, fromlen);
}

uint32_t setsockopt_c(uint32_t s, uint32_t level, uint32_t opt, uint32_t val, uint32_t len)
{
    NEED_STARTED(); NEED_SOCK(k, s);
    if (!val || len < 1) return fail(10014);
    int v = len >= 4 ? (int)rd32(val) : *(uint8_t *)G(val);
    if (level == 0xFFFF) {                                          /* SOL_SOCKET (same value on the host) */
        switch (opt) {
        case 0x4: case 0x8: case 0x20: case 0x1001: case 0x1002:    /* REUSEADDR, KEEPALIVE, BROADCAST, SNDBUF, RCVBUF */
            if (setsockopt(k->fd, SOL_SOCKET, (int)opt, &v, sizeof v)) return fail_errno();
            return 0;
        case 0x80: {                                                /* SO_LINGER: u_short onoff, u_short seconds */
            if (len < 4) return fail(10014);
            struct linger l = {(int)(rd32(val) & 0xFFFF), (int)(rd32(val) >> 16)};
            if (setsockopt(k->fd, SOL_SOCKET, SO_LINGER, &l, sizeof l)) return fail_errno();
            return 0;
        }
        case 0xFF7F: {                                              /* SO_DONTLINGER */
            struct linger l = {!v, 0};
            if (setsockopt(k->fd, SOL_SOCKET, SO_LINGER, &l, sizeof l)) return fail_errno();
            return 0;
        }
        case 0x1005: case 0x1006: {                                 /* SNDTIMEO, RCVTIMEO: milliseconds */
            struct timeval tv = {(uint32_t)v / 1000, ((uint32_t)v % 1000) * 1000};
            if (setsockopt(k->fd, SOL_SOCKET, opt == 0x1005 ? SO_SNDTIMEO : SO_RCVTIMEO, &tv, sizeof tv)) return fail_errno();
            return 0;
        }
        }
    } else if (level == 6 && opt == 1) {                            /* IPPROTO_TCP, TCP_NODELAY */
        if (setsockopt(k->fd, IPPROTO_TCP, TCP_NODELAY, &v, sizeof v)) return fail_errno();
        return 0;
    } else if (level == 0 && opt == 4) {                            /* IPPROTO_IP, IP_TTL */
        if (setsockopt(k->fd, IPPROTO_IP, IP_TTL, &v, sizeof v)) return fail_errno();
        return 0;
    }
    hp_unsupported("setsockopt", "level 0x%x option 0x%x (value %d)", level, opt, v);
}

uint32_t ioctlsocket_c(uint32_t s, uint32_t cmd, uint32_t argp)
{
    NEED_STARTED(); NEED_SOCK(k, s);
    if (!argp) return fail(10014);
    if (cmd == 0x8004667Eu) {                                       /* FIONBIO */
        int fl = fcntl(k->fd, F_GETFL);
        int on = rd32(argp) != 0;
        if (fcntl(k->fd, F_SETFL, on ? fl | O_NONBLOCK : fl & ~O_NONBLOCK)) return fail_errno();
        k->nonblocking = on;
        return 0;
    }
    if (cmd == 0x4004667Fu) {                                       /* FIONREAD */
        int n = 0;
        if (ioctl(k->fd, FIONREAD, &n)) return fail_errno();
        wr32(argp, (uint32_t)n);
        return 0;
    }
    hp_unsupported("ioctlsocket", "command 0x%08x", cmd);
}

/* ---- select on Winsock fd_sets (u_int fd_count; SOCKET fd_array[64]) ---- */

uint32_t __WSAFDIsSet_c(uint32_t s, uint32_t set)
{
    if (!set) return 0;
    uint32_t n = rd32(set);
    for (uint32_t i = 0; i < n && i < 64; i++) if (rd32(set + 4 + 4 * i) == s) return 1;
    return 0;
}

uint32_t select_c(uint32_t nfds, uint32_t rset, uint32_t wset, uint32_t eset, uint32_t timeout)
{
    (void)nfds;                                                     /* ignored by Winsock */
    NEED_STARTED();
    struct pollfd p[192];
    uint32_t who[192], which[192], np = 0;
    uint32_t sets[3] = {rset, wset, eset};
    for (int k = 0; k < 3; k++) {
        if (!sets[k]) continue;
        uint32_t n = rd32(sets[k]);
        if (n > 64) return fail(10022);
        for (uint32_t i = 0; i < n; i++) {
            uint32_t s = rd32(sets[k] + 4 + 4 * i);
            sock *so = S(s);
            if (!so) return fail(10038);
            p[np] = (struct pollfd){so->fd, (short)(k == 0 ? POLLIN : k == 1 ? POLLOUT : POLLPRI), 0};
            who[np] = s; which[np] = (uint32_t)k; np++;
        }
    }
    if (!np) {
        if (!timeout) return fail(10022);                           /* Winsock: no sockets and no timeout is invalid */
    }
    int ms = -1;
    if (timeout) ms = (int)(rd32(timeout) * 1000 + rd32(timeout + 4) / 1000);
    int r = poll(np ? p : NULL, np, ms);
    if (r < 0) return fail_errno();
    uint32_t total = 0, keep[3][64], nk[3] = {0, 0, 0};
    for (uint32_t i = 0; i < np; i++) {
        short ev = p[i].revents;
        sock *so = S(who[i]);
        int ready = 0;
        if (which[i] == 0) ready = (ev & (POLLIN | POLLHUP | POLLERR)) != 0;
        else if (which[i] == 1) {
            ready = (ev & POLLOUT) && !(ev & POLLERR);
            if (ready && so) so->connecting = 0;
        } else {
            ready = (ev & POLLPRI) != 0;
            if (so && so->connecting && (ev & (POLLERR | POLLHUP))) {   /* a failed connect is an exception */
                int err = 0; socklen_t l = sizeof err;
                getsockopt(so->fd, SOL_SOCKET, SO_ERROR, &err, &l);
                ready = err != 0;
            }
        }
        if (ready) { keep[which[i]][nk[which[i]]++] = who[i]; total++; }
    }
    for (int k = 0; k < 3; k++) {
        if (!sets[k]) continue;
        wr32(sets[k], nk[k]);
        for (uint32_t i = 0; i < nk[k]; i++) wr32(sets[k] + 4 + 4 * i, keep[k][i]);
    }
    return total;
}

/* ---- byte order and addresses ---- */

uint32_t htonl_c(uint32_t v) { return __builtin_bswap32(v); }
uint32_t ntohl_c(uint32_t v) { return __builtin_bswap32(v); }
uint32_t htons_c(uint32_t v) { return __builtin_bswap16((uint16_t)v); }
uint32_t ntohs_c(uint32_t v) { return __builtin_bswap16((uint16_t)v); }

uint32_t inet_addr_c(uint32_t cp)
{
    if (!cp) return 0xFFFFFFFFu;
    const char *s = G(cp);
    if (!*s) return 0;                                              /* XP answers 0 for "" */
    struct in_addr a;
    if (!inet_aton(s, &a)) return 0xFFFFFFFFu;                      /* INADDR_NONE */
    return a.s_addr;
}

uint32_t inet_ntoa_c(uint32_t addr)
{
    if (!ntoa_buf) ntoa_buf = halopad_heap_alloc(16, 1);            /* one buffer per thread, as Winsock's */
    struct in_addr a = {addr};
    snprintf(G(ntoa_buf), 16, "%s", inet_ntoa(a));
    return ntoa_buf;
}

uint32_t gethostname_c(uint32_t name, uint32_t len)
{
    NEED_STARTED();
    char h[256];
    if (gethostname(h, sizeof h)) return fail_errno();
    char *dot = strchr(h, '.');
    if (dot) *dot = 0;                                              /* Windows gives the short name */
    if (!name || strlen(h) + 1 > len) return fail(10014);
    strcpy(G(name), h);
    return 0;
}

/* HOSTENT in a per-thread guest buffer: h_name, h_aliases, h_addrtype, h_length, h_addr_list */
uint32_t gethostbyname_c(uint32_t name)
{
    if (!started) { fail(10093); return 0; }
    if (!name) { fail(10014); return 0; }
    /* Windows always resolves its own host name, to the addresses of its IPv4 interfaces (Halo's
       network start-up, 0x4415c0, looks up the name gethostname gave and uses h_addr_list[0]
       without a check). macOS resolves only the .local form, so answer our own name here. */
    uint32_t own[16], nown = 0;
    {
        char h[256];
        const char *want = G(name);
        if (!gethostname(h, sizeof h)) {
            char *dot = strchr(h, '.');
            size_t shortlen = dot ? (size_t)(dot - h) : strlen(h);
            if (!strcasecmp(want, h) || (strlen(want) == shortlen && !strncasecmp(want, h, shortlen))) {
                struct ifaddrs *ifs = NULL;
                if (!getifaddrs(&ifs)) {
                    for (struct ifaddrs *i = ifs; i && nown < 16; i = i->ifa_next)
                        if (i->ifa_addr && i->ifa_addr->sa_family == AF_INET && (i->ifa_flags & IFF_UP) && !(i->ifa_flags & IFF_LOOPBACK))
                            own[nown++] = ((struct sockaddr_in *)i->ifa_addr)->sin_addr.s_addr;
                    freeifaddrs(ifs);
                }
                if (!nown) own[nown++] = htonl(0x7F000001);         /* no network: Windows gives the loopback address */
            }
        }
    }
    struct addrinfo hints = {0}, *res = NULL;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if (!nown && lan_only() && strcasecmp(G(name), "localhost")) { fail(11001); return 0; }   /* offline: no DNS */
    if (!nown) {
        int e = getaddrinfo(G(name), NULL, &hints, &res);
        if (e || !res) { fail(e == EAI_AGAIN ? 11002 : 11001); return 0; }   /* WSATRY_AGAIN, WSAHOST_NOT_FOUND */
    }
    if (!hostent_buf) hostent_buf = halopad_heap_alloc(1024, 1);
    uint32_t b = hostent_buf, addrs = b + 16 + 4, list = b + 16 + 4 + 4 * 16, str = b + 16 + 4 + 4 * 16 + 4 * 17;
    uint32_t n = 0;
    uint32_t found[16], nfound = 0;
    for (uint32_t k = 0; k < nown; k++) found[nfound++] = own[k];
    for (struct addrinfo *r = res; r && nfound < 16; r = r->ai_next) found[nfound++] = ((struct sockaddr_in *)r->ai_addr)->sin_addr.s_addr;
    for (uint32_t k = 0; k < nfound; k++) {
        uint32_t a = found[k];
        int dup = 0;
        for (uint32_t i = 0; i < n; i++) if (rd32(addrs + 4 * i) == a) dup = 1;
        if (dup) continue;
        wr32(addrs + 4 * n, a);
        wr32(list + 4 * n, addrs + 4 * n);
        n++;
    }
    wr32(list + 4 * n, 0);
    snprintf(G(str), 1024 - (str - b), "%s", res && res->ai_canonname ? res->ai_canonname : (const char *)G(name));
    if (res) freeaddrinfo(res);
    wr32(b + 16, 0);                                                /* h_aliases: an empty list */
    wr32(b, str); wr32(b + 4, b + 16); wr16(b + 8, 2); wr16(b + 10, 4); wr32(b + 12, list);
    return b;
}
