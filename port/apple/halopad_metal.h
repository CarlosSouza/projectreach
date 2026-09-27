/* HaloPad Apple host: C interface to the Metal layer (port/apple/halopad_metal.m). All
 * enum fields hold Metal enum values; the Direct3D -> Metal mapping is done by the caller
 * (port/runtime/halopad_d3d9_device.c). Returned objects are retained; release them with
 * halopad_metal_release. */
#ifndef HALOPAD_METAL_H
#define HALOPAD_METAL_H
#include <stdint.h>

typedef struct { uint8_t stream, format, reg, pad; uint32_t offset; } hp_vattr;   /* format: MTLVertexFormat */

typedef struct {
    const char *vs_msl, *ps_msl;
    uint32_t nattr;
    hp_vattr attr[16];
    uint32_t stride[16];
    uint32_t constant_regs;          /* shader inputs with no element: constant (0, 0, 0, 1) */
    uint8_t blend, src_rgb, dst_rgb, op_rgb, src_a, dst_a, op_a, write_mask;
    uint8_t depth;                   /* the target has a depth/stencil attachment */
} hp_pipeline_desc;

typedef struct {
    uint8_t depth_write, depth_func;          /* MTLCompareFunction (always when depth testing is off) */
    uint8_t stencil, fail[2], zfail[2], pass[2], func[2];   /* [0] front, [1] back */
    uint32_t read_mask, write_mask;
} hp_depth_desc;

typedef struct {
    uint8_t min, mag, mip, u, v, w, border, anisotropy;   /* MTLSamplerMinMagFilter, MipFilter, AddressMode, BorderColor */
    float lod_min;
} hp_sampler_desc;

typedef struct {
    void *pipeline, *depth_state;
    uint8_t cull, front_cw, lines;            /* MTLCullMode, front face clockwise, wireframe */
    float slope_bias;
    uint32_t stencil_ref;
    float blend_color[4];
    double viewport[6];                       /* x, y, width, height, znear, zfar */
    uint8_t scissor;
    uint32_t scissor_rect[4];
    void *vbuf[16];
    uint32_t voff[16];
    const void *vs_consts, *ps_consts;
    uint32_t vs_len, ps_len;
    void *tex[16], *smp[16];
    uint8_t prim;                             /* MTLPrimitiveType */
    uint32_t start, count;                    /* vertices, or indices when ibuf is set */
    void *ibuf;
    uint8_t index32;
    uint32_t index_offset;
    int32_t base_vertex;
} hp_draw_desc;

void *halopad_metal_pipeline(const hp_pipeline_desc *d, char *err, uint32_t errlen);
void *halopad_metal_depth_state(const hp_depth_desc *d);
void *halopad_metal_sampler(const hp_sampler_desc *d);
void *halopad_metal_buffer(const void *data, uint32_t length);
void halopad_metal_release(void *o);
/* type: 2 = 2D, 3 = cube, 4 = 3D; format: MTLPixelFormat; swizzle: MTLTextureSwizzle x4 */
void *halopad_metal_texture(int type, uint32_t format, uint32_t w, uint32_t h, uint32_t d, uint32_t levels, const uint8_t swizzle[4]);
void halopad_metal_texture_upload(void *tex, uint32_t level, uint32_t slice, const void *data, uint32_t bytes_per_row,
                                  uint32_t bytes_per_image, uint32_t w, uint32_t h, uint32_t d);
void halopad_metal_draw(void *target, const hp_draw_desc *d);
#endif
