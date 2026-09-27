# Graphics contract: what HaloPad's Direct3D 9 reports

Halo decides its rendering path from what Direct3D 9 tells it: the adapter identity (matched against the game's `config.txt` card database), display modes, format support and `D3DCAPS9`. HaloPad's answers follow two rules.

1. **Identity selects Halo's own tuning.** HaloPad reports an ATI Radeon 9700 PRO (vendor `0x1002`, device `0x4e44`), which `config.txt` lists with tested settings (decal depth bias, no forced shader downgrade). The driver version is 6.14.10.6467: newer than the entry's "old driver" threshold (6.14.10.6368), and not the `ForceShader=14` build (6.14.10.6378) or the invalid builds. This is a label for Halo's card database, not a claim about the Mac's GPU.
2. **Capabilities are promises about HaloPad's Metal layer.** A capability bit, limit, format or multisample type is reported only if the Metal implementation provides it, capped at the Radeon 9700 PRO class Halo was tuned for. Anything else is reported absent (`D3DERR_NOTAVAILABLE`, bit clear), which is Direct3D's defined way to say "not supported" and makes Halo choose another path.

Implementation: `port/runtime/halopad_d3d9.c`. The fields Halo reads are listed in [D3D9-INVENTORY.md](D3D9-INVENTORY.md).

## Fields Halo reads

| Field | Value | Why |
|---|---|---|
| `PixelShaderVersion` | 2.0 (`0xffff0200`) | Halo loads `EffectCollection_ps_2_0`. ps_2_0 bytecode has regular register semantics and translates to Metal more directly than ps_1_x's texture-addressing opcodes. |
| `VertexShaderVersion` | 2.0 (`0xfffe0200`) | Covers Halo's `vsh.enc` (vs_1_1) and matches the card. |
| `RasterCaps` | depth bias, slope-scaled depth bias, anisotropy, W and Z fog, range fog, table and vertex fog, colour perspective, scissor, mip LOD bias, Z test | Metal provides depth bias and slope scale (`setDepthBias:slopeScale:clamp:`), anisotropic sampling, scissor and LOD bias; fog is computed in the translated shaders. Halo tests the two depth-bias bits (`0x06000000`). |
| `MaxSimultaneousTextures`, `MaxTextureBlendStages` | 8, 8 | The card's limit; Metal allows more. |
| `MaxActiveLights` | 8 | Fixed-function lighting in translated shaders; the card's limit. |
| `MaxStreams` | 16 | Metal vertex buffer slots; the card's value. Halo also overwrites this field with 1 in some paths. |
| `MaxAnisotropy` | 16 | Metal's maximum and the card's. |
| `TextureAddressCaps` | wrap, mirror, clamp, border, mirror-once, independent U/V | Metal sampler address modes, including clamp to border colour. Halo tests the border bit (`0x08`). |
| `SrcBlendCaps` / `DestBlendCaps` | all D3D9 factors (`0x3fff`) | Metal blend factors; `BOTHSRCALPHA` forms map to source/inverse-source alpha. |
| `TextureCaps` | perspective, alpha, projected, cube, volume, mipmapped 2D/cube/volume; **no** power-of-two restriction | Metal textures have no power-of-two limits. |
| `TextureFilterCaps` | point/linear/anisotropic min and mag, point/linear mip | Metal sampler filters. |
| `Caps2` | dynamic textures, full-screen gamma, automatic mipmap generation | Gamma ramps are applied at presentation; Metal generates mipmaps. |
| `DevCaps` | hardware T&L, hardware rasterization, the draw-primitive and texture-memory bits; **not** a pure device, no patches | Halo's word read at `DevCaps+2` sees HWTRANSFORMANDLIGHT and HWRASTERIZATION. Not being a pure device keeps `Get*` state calls valid. |

## Formats and modes

- **Adapter modes.** The standard modes up to the Mac's main display size, plus the display's own size, at 60 Hz, in `X8R8G8B8` and `R5G6B5`.
- **Back buffers and render targets.** `X8R8G8B8`, `A8R8G8B8` and `R5G6B5`.
- **Depth.** `D24S8`, `D24X8` and `D16`.
- **Textures.** `A8R8G8B8`, `X8R8G8B8`, `R5G6B5`, `X1R5G5B5`, `A1R5G5B5`, `A4R4G4B4`, `A8`, `L8`, `A8L8`, `V8U8`, `Q8W8V8U8`, `DXT1` to `DXT5`.
- **Multisampling.** Only `D3DMULTISAMPLE_NONE` until the Metal layer resolves multisampled targets.
- **Everything else** is reported not available.
