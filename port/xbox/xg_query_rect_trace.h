#ifndef XG_QUERY_RECT_TRACE_H
#define XG_QUERY_RECT_TRACE_H
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Diagnostic for the pinned guest's immediate quad: 16 float4 attributes per
 * vertex, position first. No GL calls, guest writes or retained guest pointers.
 * Reject other layouts/draws. IDs are GL query objects, NOT Halo flare indices.
 * Collisions evict evidence rather than associate it with the wrong query.
 * At most one result per ID per second, 4096 rows total, exact opt-in only. */
static struct {
    int enabled, checked, active, uploads, draws, valid;
    unsigned id, rows;
    float vertex[4][4];
    struct { unsigned id; int valid; time_t logged; float vertex[4][4]; } slots[256];
} xg_qr;

static int xg_query_rect_enabled(void)
{
    if (!xg_qr.checked) {
        const char *value = getenv("XG_TRACE_QUERY_RECTS");
        xg_qr.checked = 1;
        xg_qr.enabled = value && !strcmp(value, "1");
    }
    return xg_qr.enabled && xg_qr.rows < 4096;
}

static void xg_query_rect_begin(unsigned target, unsigned id)
{
    if (!xg_query_rect_enabled()) return;
    xg_qr.active = target == 0x8c2f; /* GL_ANY_SAMPLES_PASSED */
    xg_qr.id = id;
    xg_qr.uploads = xg_qr.draws = xg_qr.valid = 0;
    xg_qr.slots[id % 256].valid = 0;
}

static void xg_query_rect_upload(unsigned target, unsigned size, const void *data)
{
    if (!xg_query_rect_enabled() || !xg_qr.active || target != 0x8892) return;
    xg_qr.uploads++;
    if (xg_qr.uploads != 1 || size != 1024 || !data) { xg_qr.valid = 0; return; }
    xg_qr.valid = 1;
    for (unsigned i = 0; i < 4; i++) {
        memcpy(xg_qr.vertex[i], (const unsigned char *)data + i * 256, 16);
        for (unsigned j = 0; j < 4; j++)
            if (!isfinite(xg_qr.vertex[i][j])) xg_qr.valid = 0;
    }
}

static void xg_query_rect_draw(unsigned mode, int first, int count)
{
    if (!xg_query_rect_enabled() || !xg_qr.active) return;
    xg_qr.draws++;
    if (mode != 6 /* GL_TRIANGLE_FAN */ || first || count != 4)
        xg_qr.valid = 0;
}

static void xg_query_rect_end(unsigned target)
{
    if (!xg_query_rect_enabled() || !xg_qr.active || target != 0x8c2f) return;
    xg_qr.active = 0;
    float (*v)[4] = xg_qr.vertex;
    if (!xg_qr.valid || xg_qr.uploads != 1 || xg_qr.draws != 1) return;
    if (v[0][0] != v[3][0] || v[1][0] != v[2][0] ||
        v[0][1] != v[1][1] || v[2][1] != v[3][1] ||
        v[0][0] >= v[1][0] || v[0][1] >= v[2][1]) return;
    for (unsigned i = 0; i < 4; i++)
        if (v[i][2] != v[0][2] || v[i][3] != 1 ||
            fabsf(v[i][0]) > 32767 || fabsf(v[i][1]) > 32767) return;
    unsigned slot = xg_qr.id % 256;
    xg_qr.slots[slot].id = xg_qr.id;
    xg_qr.slots[slot].valid = 1;
    memcpy(xg_qr.slots[slot].vertex, v, sizeof(xg_qr.vertex));
}

static void xg_query_rect_result(unsigned id, unsigned name, unsigned value)
{
    if (!xg_query_rect_enabled() || name != 0x8866) return;
    unsigned slot = id % 256;
    if (!xg_qr.slots[slot].valid || xg_qr.slots[slot].id != id) return;
    time_t now = time(NULL);
    if (xg_qr.slots[slot].logged == now) return;
    xg_qr.slots[slot].logged = now;
    float (*v)[4] = xg_qr.slots[slot].vertex;
    xg_log("query rect: time %lld id %u rect %.9g %.9g %.9g %.9g z %.9g area %.9g result %u",
        (long long)now, id, v[0][0], v[0][1], v[2][0], v[2][1], v[0][2],
        (double)(v[2][0] - v[0][0]) * (v[2][1] - v[0][1]), value);
    xg_qr.rows++;
}
#endif
