/* Winsock test (G5/G6): WS2_32/WSOCK32 the way Halo's GameSpy library uses them, on real
 * loopback sockets. Calls go through the guest addresses GetProcAddress gives (the delay-
 * load path). A host TCP listener stands in for a master server.
 * Linked in place of the core's main by scripts/run-core.py --main tests/halo_winsock_test.c. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#define PTROFS_64BIT 1
#include "llasm_cpu.h"

extern uint64_t halopad_guest_base;
extern int halopad_guest_harness_heap;
extern _Thread_local _cpu *halopad_cpu;
uint32_t halopad_guest_init(const char *image_path, uint32_t image_base);
void halopad_thread_init(uint32_t stack_base, uint32_t stack_limit, uint32_t image_base);
void halopad_vm_mark(uint32_t base, uint32_t size);
uint32_t halopad_call_guest(uint32_t va, uint32_t nargs, const uint32_t *args);
uint32_t halopad_heap_alloc(uint32_t size, int zero);
void *halopad_guest_ptr(uint32_t guest);
uint32_t LoadLibraryA_c(uint32_t name);
uint32_t GetProcAddress_c(uint32_t module, uint32_t name);

static int failures;
static uint32_t rd(uint32_t g) { uint32_t v; memcpy(&v, halopad_guest_ptr(g), 4); return v; }
static uint32_t str(const char *s) { uint32_t g = halopad_heap_alloc((uint32_t)strlen(s) + 1, 0); strcpy(halopad_guest_ptr(g), s); return g; }
static char *P(uint32_t g) { return halopad_guest_ptr(g); }
static void check(const char *what, uint32_t got, uint32_t want)
{
    printf("%-66s %s (got 0x%x, want 0x%x)\n", what, got == want ? "PASS" : "FAIL", got, want);
    failures += got != want;
}
static uint32_t ws2, wsock;
static uint32_t api(const char *name, uint32_t n, const uint32_t *args)
{
    static const char *const w32[] = {"WSACleanup", "__WSAFDIsSet", "bind", "closesocket", "connect", "getsockname", "htonl", "htons",
                                      "ioctlsocket", "ntohl", "ntohs", "recv", "select", "send", "shutdown"};   /* Halo's WSOCK32 imports */
    uint32_t mod = ws2;
    for (size_t i = 0; i < sizeof w32 / sizeof w32[0]; i++) if (!strcmp(name, w32[i])) mod = wsock;
    uint32_t va = GetProcAddress_c(mod, str(name));
    if (!va) { printf("no export %s\n", name); exit(2); }
    return halopad_call_guest(va, n, args);
}
#define API(name, ...) api(name, sizeof((uint32_t[]){__VA_ARGS__}) / 4, (uint32_t[]){__VA_ARGS__})
#define API0(name) api(name, 0, NULL)
#define ERR() API0("WSAGetLastError")

static uint32_t addr(uint32_t ip_be, uint16_t port_be)          /* a Winsock sockaddr_in */
{
    uint32_t a = halopad_heap_alloc(16, 1);
    uint8_t *p = halopad_guest_ptr(a);
    p[0] = 2; memcpy(p + 2, &port_be, 2); memcpy(p + 4, &ip_be, 4);
    return a;
}
static uint32_t fdset(int n, const uint32_t *s)
{
    uint32_t f = halopad_heap_alloc(4 + 64 * 4, 1);
    memcpy(halopad_guest_ptr(f), &n, 4);
    memcpy((uint8_t *)halopad_guest_ptr(f) + 4, s, 4 * n);
    return f;
}
static uint32_t tv(uint32_t sec, uint32_t usec) { uint32_t t = halopad_heap_alloc(8, 1); memcpy(halopad_guest_ptr(t), (uint32_t[]){sec, usec}, 8); return t; }

int main(void)
{
    const char *image = getenv("HALOPAD_IMAGE");
    if (!image) return 2;
    halopad_guest_harness_heap = 0;
    uint32_t top = halopad_guest_init(image, 0x400000);
    halopad_thread_init(0x00300000, 0x00100000, 0x400000);
    halopad_vm_mark(0x00100000, 0x00200000);
    halopad_vm_mark(0x400000, 0x42C000);
    halopad_vm_mark(0x7FFD0000, 0x00030000);
    _cpu cpu;
    memset(&cpu, 0, sizeof cpu);
    cpu._esp = top;
    cpu._pointer_offset = halopad_guest_base;
    cpu._st_cw = 0x27F;
    halopad_cpu = &cpu;
    ws2 = LoadLibraryA_c(str("ws2_32.dll"));
    wsock = LoadLibraryA_c(str("wsock32.dll"));
    check("LoadLibraryA ws2_32.dll and wsock32.dll", ws2 && wsock, 1);
    const uint32_t LO = htonl(INADDR_LOOPBACK);

    check("socket before WSAStartup: WSANOTINITIALISED", API("socket", 2, 2, 0) == 0xFFFFFFFF && ERR() == 10093, 1);
    uint32_t wd = halopad_heap_alloc(400, 1);
    check("WSAStartup(2.2)", API("WSAStartup", 0x0202, wd), 0);
    check("  wVersion 2.2, wHighVersion 2.2", rd(wd), 0x02020202);

    /* UDP, as game traffic */
    uint32_t a = API("socket", 2, 2, 17), b = API("socket", 2, 2, 0);
    check("socket(AF_INET, SOCK_DGRAM) x2", a != 0xFFFFFFFF && b != 0xFFFFFFFF && a != b, 1);
    check("bind to 127.0.0.1:0", API("bind", a, addr(LO, 0), 16) | API("bind", b, addr(LO, 0), 16), 0);
    uint32_t na = halopad_heap_alloc(16, 1), nb = halopad_heap_alloc(16, 1), ln = halopad_heap_alloc(4, 1);
    memcpy(halopad_guest_ptr(ln), (uint32_t[]){16}, 4); API("getsockname", a, na, ln);
    memcpy(halopad_guest_ptr(ln), (uint32_t[]){16}, 4); API("getsockname", b, nb, ln);
    uint16_t pa; memcpy(&pa, P(na) + 2, 2);
    check("getsockname: AF_INET, an assigned port, 16 bytes", P(na)[0] == 2 && pa != 0 && rd(ln) == 16, 1);
    uint32_t one = halopad_heap_alloc(4, 1), big = halopad_heap_alloc(4, 1);
    memcpy(P(one), (uint32_t[]){1}, 4); memcpy(P(big), (uint32_t[]){65536}, 4);
    check("setsockopt SO_BROADCAST, SO_RCVBUF", API("setsockopt", a, 0xFFFF, 0x20, one, 4) | API("setsockopt", a, 0xFFFF, 0x1002, big, 4), 0);
    uint32_t hello = str("hello");
    check("sendto b -> a", API("sendto", b, hello, 5, 0, na, 16), 5);
    uint32_t rs = fdset(1, &a);
    check("select(read) sees it", API("select", 0, rs, 0, 0, tv(1, 0)), 1);
    check("  __WSAFDIsSet", API("__WSAFDIsSet", a, rs), 1);
    uint32_t buf = halopad_heap_alloc(256, 1), from = halopad_heap_alloc(16, 1), fl = halopad_heap_alloc(4, 1);
    memcpy(P(fl), (uint32_t[]){16}, 4);
    check("recvfrom: 5 bytes", API("recvfrom", a, buf, 256, 0, from, fl), 5);
    check("  \"hello\" from b's address", memcmp(P(buf), "hello", 5) == 0 && memcmp(P(from), P(nb), 8) == 0 && rd(fl) == 16, 1);
    uint32_t hundred = halopad_heap_alloc(100, 0);
    memset(P(hundred), 'x', 100);
    API("sendto", b, hundred, 100, 0, na, 16);
    memset(P(buf), 0, 256);
    check("a datagram longer than the buffer: SOCKET_ERROR, WSAEMSGSIZE", API("recvfrom", a, buf, 10, 0, 0, 0) == 0xFFFFFFFF && ERR() == 10040, 1);
    check("  the buffer holds its start", P(buf)[9] == 'x' && P(buf)[10] == 0, 1);
    check("ioctlsocket(FIONBIO, 1)", API("ioctlsocket", a, 0x8004667E, one), 0);
    check("  recvfrom with nothing queued: WSAEWOULDBLOCK", API("recvfrom", a, buf, 256, 0, 0, 0) == 0xFFFFFFFF && ERR() == 10035, 1);
    rs = fdset(1, &a);
    check("select with nothing to read times out: 0", API("select", 0, rs, 0, 0, tv(0, 50000)), 0);
    check("  and empties the set", rd(rs), 0);
    API("sendto", b, hello, 5, 0, na, 16);
    usleep(20000);
    uint32_t nread = halopad_heap_alloc(4, 1);
    check("ioctlsocket(FIONREAD) counts queued data", API("ioctlsocket", a, 0x4004667F, nread) == 0 && rd(nread) >= 5, 1);
    check("closesocket x2", API("closesocket", a) | API("closesocket", b), 0);
    check("closesocket again: WSAENOTSOCK", API("closesocket", a) == 0xFFFFFFFF && ERR() == 10038, 1);

    /* TCP, as the master-server list */
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in la = {sizeof la, AF_INET, 0, {LO}};
    bind(lfd, (struct sockaddr *)&la, sizeof la);
    listen(lfd, 4);
    socklen_t ll = sizeof la;
    getsockname(lfd, (struct sockaddr *)&la, &ll);
    uint32_t t = API("socket", 2, 1, 6);
    check("socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)", t != 0xFFFFFFFF, 1);
    check("connect (blocking)", API("connect", t, addr(LO, la.sin_port), 16), 0);
    int cfd = accept(lfd, NULL, NULL);
    check("send \"ping\"", API("send", t, str("ping"), 4, 0), 4);
    char hb[16] = {0};
    check("  the server reads it", read(cfd, hb, sizeof hb) == 4 && !memcmp(hb, "ping", 4), 1);
    write(cfd, "pong", 4);
    check("recv \"pong\"", API("recv", t, buf, 256, 0) == 4 && !memcmp(P(buf), "pong", 4), 1);
    check("shutdown(SD_BOTH)", API("shutdown", t, 2), 0);
    check("  the server sees the end", read(cfd, hb, sizeof hb), 0);
    close(cfd);
    API("closesocket", t);
    t = API("socket", 2, 1, 6);
    API("ioctlsocket", t, 0x8004667E, one);
    check("non-blocking connect: WSAEWOULDBLOCK", API("connect", t, addr(LO, la.sin_port), 16) == 0xFFFFFFFF && ERR() == 10035, 1);
    uint32_t ws = fdset(1, &t);
    check("  select(write) once connected", API("select", 0, 0, ws, 0, tv(2, 0)), 1);
    cfd = accept(lfd, NULL, NULL);
    close(cfd);
    API("closesocket", t);
    close(lfd);                                                     /* the port is now closed */
    t = API("socket", 2, 1, 6);
    API("ioctlsocket", t, 0x8004667E, one);
    API("connect", t, addr(LO, la.sin_port), 16);
    ws = fdset(1, &t);
    uint32_t es = fdset(1, &t);
    check("refused non-blocking connect: in select's exception set", API("select", 0, 0, ws, es, tv(2, 0)) == 1 && rd(es) == 1 && rd(ws) == 0, 1);
    API("closesocket", t);
    t = API("socket", 2, 1, 6);
    check("refused blocking connect: WSAECONNREFUSED", API("connect", t, addr(LO, la.sin_port), 16) == 0xFFFFFFFF && ERR() == 10061, 1);
    API("closesocket", t);

    /* byte order and names */
    check("htons(0x1234)", API("htons", 0x1234), 0x3412);
    check("ntohl(0x01020304)", API("ntohl", 0x01020304), 0x04030201);
    check("inet_addr(\"192.168.1.2\")", API("inet_addr", str("192.168.1.2")), 0x0201A8C0);
    check("inet_addr(\"not.an.address\"): INADDR_NONE", API("inet_addr", str("not.an.address")), 0xFFFFFFFF);
    check("inet_ntoa", strcmp(P(API("inet_ntoa", 0x0201A8C0)), "192.168.1.2"), 0);
    uint32_t hn = halopad_heap_alloc(256, 1);
    check("gethostname", API("gethostname", hn, 256) == 0 && P(hn)[0] != 0 && !strchr(P(hn), '.'), 1);
    uint32_t he = API("gethostbyname", str("localhost"));
    check("gethostbyname(\"localhost\"): AF_INET, 4-byte addresses, 127.0.0.1",
          he && (rd(he + 8) & 0xFFFF) == 2 && (rd(he + 8) >> 16) == 4 && rd(rd(rd(he + 12))) == LO, 1);
    check("gethostbyname of an unknown name: WSAHOST_NOT_FOUND", API("gethostbyname", str("no-such-host.invalid")) == 0 && ERR() == 11001, 1);
    check("WSACleanup", API0("WSACleanup"), 0);
    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 1 : 0;
}
