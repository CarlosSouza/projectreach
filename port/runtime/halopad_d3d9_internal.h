/* HaloPad Direct3D 9: state shared by halopad_d3d9_device.c, _resources.c and _draw.c. */
#ifndef HALOPAD_D3D9_INTERNAL_H
#define HALOPAD_D3D9_INTERNAL_H
#include <stdint.h>

#define MAXLEVELS 14

enum { R_TEXTURE, R_SURFACE, R_VB, R_IB, R_DECL, R_VS, R_PS };
typedef struct res {
    int kind;
    uint32_t guest, device;
    uint32_t usage, format, pool, priority, fvf, lod, autogen_filter;
    uint32_t width, height, levels;
    uint32_t mem[MAXLEVELS], pitch[MAXLEVELS], size[MAXLEVELS], lw[MAXLEVELS], lh[MAXLEVELS];
    uint8_t locked[MAXLEVELS], dirty[MAXLEVELS];
    uint32_t surface[MAXLEVELS];
    uint32_t parent, level;                     /* surfaces of a texture */
    uint32_t length, buf, locks;                /* buffers */
    uint32_t count, *tokens;                    /* declarations (elements) and shaders (tokens) */
    void *native;                               /* Metal object, created on first use */
    char *msl;                                  /* vertex shaders: translated source */
    uint8_t in_usage[16], in_index[16], in_used[16];   /* vertex shaders: dcl_<usage><index> v<n> */
    struct { uint8_t key[33]; char *msl; } variant[8];  /* pixel shaders: source per sampler key */
    uint32_t nvariant;
} res;


#define NRS 210
#define NTSS 33
#define NSS 14
#define NSAMPLERS 16


typedef struct {
    uint32_t guest, d3d, window, behavior, pp[14];
    uint32_t rs[NRS], tss[8][NTSS], ss[NSAMPLERS][NSS];
    float transform[512][16];
    uint32_t viewport[6];
    int in_scene, software_vp, cursor_shown;
    uint16_t gamma[3][256];
    void *target;
    /* bindings (each holds a bind on the object) and shader constants */
    uint32_t texture[16], stream[16], stream_offset[16], stream_stride[16], indices, decl, fvf, vs, ps;
    float vsf[256][4], psf[224][4];
    int32_t vsi[16][4], psi[16][4];
    uint32_t vsb[16], psb[16];
    /* fixed-function lighting: D3DMATERIAL9 and D3DLIGHT9 (as Halo passes them), enable flags */
    float material[17];
    struct { uint32_t index, set, enabled; float light[26]; } light[16];
    uint32_t nlight;
} device;

#endif
