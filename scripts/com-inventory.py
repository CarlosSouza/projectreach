#!/usr/bin/env python3
"""Inventory COM method calls through interface pointers held in globals (G3/G4).

Scans every audited function's straight-line code for
    reg = [global]; vt = [reg]; call [vt + offset]
and counts (global, method offset). Named interfaces map offsets to methods (d3d9.h
order). This is a lower bound: calls through pointers passed in registers or held in
structures are not seen. Writes docs/D3D9-INVENTORY.md and
generated/analysis/<profile>/com-inventory.json.

Usage: .venv/bin/python scripts/com-inventory.py
"""
import collections
import json
import pathlib

import capstone
import pefile
from capstone import x86 as X

ROOT = pathlib.Path(__file__).resolve().parents[1]
A = ROOT / 'generated' / 'analysis' / 'custom-en-1.0.10.0621'
EXE = ROOT / 'ref' / 'inputs' / 'custom-original' / 'haloce.exe'
DEVICE9 = 'QueryInterface AddRef Release TestCooperativeLevel GetAvailableTextureMem EvictManagedResources GetDirect3D GetDeviceCaps GetDisplayMode GetCreationParameters SetCursorProperties SetCursorPosition ShowCursor CreateAdditionalSwapChain GetSwapChain GetNumberOfSwapChains Reset Present GetBackBuffer GetRasterStatus SetDialogBoxMode SetGammaRamp GetGammaRamp CreateTexture CreateVolumeTexture CreateCubeTexture CreateVertexBuffer CreateIndexBuffer CreateRenderTarget CreateDepthStencilSurface UpdateSurface UpdateTexture GetRenderTargetData GetFrontBufferData StretchRect ColorFill CreateOffscreenPlainSurface SetRenderTarget GetRenderTarget SetDepthStencilSurface GetDepthStencilSurface BeginScene EndScene Clear SetTransform GetTransform MultiplyTransform SetViewport GetViewport SetMaterial GetMaterial SetLight GetLight LightEnable GetLightEnable SetClipPlane GetClipPlane SetRenderState GetRenderState CreateStateBlock BeginStateBlock EndStateBlock SetClipStatus GetClipStatus GetTexture SetTexture GetTextureStageState SetTextureStageState GetSamplerState SetSamplerState ValidateDevice SetPaletteEntries GetPaletteEntries SetCurrentTexturePalette GetCurrentTexturePalette SetScissorRect GetScissorRect SetSoftwareVertexProcessing GetSoftwareVertexProcessing SetNPatchMode GetNPatchMode DrawPrimitive DrawIndexedPrimitive DrawPrimitiveUP DrawIndexedPrimitiveUP ProcessVertices CreateVertexDeclaration SetVertexDeclaration GetVertexDeclaration SetFVF GetFVF CreateVertexShader SetVertexShader GetVertexShader SetVertexShaderConstantF GetVertexShaderConstantF SetVertexShaderConstantI GetVertexShaderConstantI SetVertexShaderConstantB GetVertexShaderConstantB SetStreamSource GetStreamSource SetStreamSourceFreq GetStreamSourceFreq SetIndices GetIndices CreatePixelShader SetPixelShader GetPixelShader SetPixelShaderConstantF GetPixelShaderConstantF SetPixelShaderConstantI GetPixelShaderConstantI SetPixelShaderConstantB GetPixelShaderConstantB DrawRectPatch DrawTriPatch DeletePatch CreateQuery'.split()
INTERFACES = {0x6B840C: ('IDirect3DDevice9', DEVICE9)}   # set by IDirect3D9::CreateDevice (0x6bd168 holds IDirect3D9)


def scan():
    pe = pefile.PE(str(EXE), fast_load=True)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    text = pe.sections[0]
    base = pe.OPTIONAL_HEADER.ImageBase
    data = pe.get_data(text.VirtualAddress, text.Misc_VirtualSize)
    fns = sorted(int(k, 16) for k in json.loads((A / 'functions.json').read_text()))
    fns.append(base + text.VirtualAddress + text.Misc_VirtualSize)
    sites = collections.defaultdict(list)
    for f, nf in zip(fns, fns[1:]):
        obj, vt = {}, {}
        off = f - base - text.VirtualAddress
        for i in md.disasm(data[off:off + (nf - f)], f):
            ops = i.operands
            if i.mnemonic == 'mov' and len(ops) == 2 and ops[0].type == X.X86_OP_REG and ops[1].type == X.X86_OP_MEM:
                dst, mem = ops[0].reg, ops[1].mem
                obj.pop(dst, None)
                vt.pop(dst, None)
                if mem.base == 0 and mem.index == 0 and mem.disp >= 0x600000:
                    obj[dst] = mem.disp
                elif mem.base in obj and mem.index == 0 and mem.disp == 0:
                    vt[dst] = obj[mem.base]
                continue
            if i.mnemonic == 'call' and len(ops) == 1 and ops[0].type == X.X86_OP_MEM:
                mem = ops[0].mem
                if mem.base in vt and mem.index == 0:
                    sites[(vt[mem.base], mem.disp)].append(i.address)
                obj, vt = {}, {}
                continue
            for r in i.regs_access()[1]:
                obj.pop(r, None)
                vt.pop(r, None)
            if i.mnemonic.startswith('j') or i.mnemonic.startswith('ret'):
                obj, vt = {}, {}
    return sites


CAPS_GLOBAL = 0x75C420          # IDirect3D9::GetDeviceCaps(adapter, HAL, 0x75c420) at 0x51a414
CAPS_FIELDS = [(n, int(o)) for n, o in (x.split() for x in 'DeviceType 0,AdapterOrdinal 4,Caps 8,Caps2 12,Caps3 16,PresentationIntervals 20,CursorCaps 24,DevCaps 28,PrimitiveMiscCaps 32,RasterCaps 36,ZCmpCaps 40,SrcBlendCaps 44,DestBlendCaps 48,AlphaCmpCaps 52,ShadeCaps 56,TextureCaps 60,TextureFilterCaps 64,CubeTextureFilterCaps 68,VolumeTextureFilterCaps 72,TextureAddressCaps 76,VolumeTextureAddressCaps 80,LineCaps 84,MaxTextureWidth 88,MaxTextureHeight 92,MaxVolumeExtent 96,MaxTextureRepeat 100,MaxTextureAspectRatio 104,MaxAnisotropy 108,MaxVertexW 112,GuardBandLeft 116,GuardBandTop 120,GuardBandRight 124,GuardBandBottom 128,ExtentsAdjust 132,StencilCaps 136,FVFCaps 140,TextureOpCaps 144,MaxTextureBlendStages 148,MaxSimultaneousTextures 152,VertexProcessingCaps 156,MaxActiveLights 160,MaxUserClipPlanes 164,MaxVertexBlendMatrices 168,MaxVertexBlendMatrixIndex 172,MaxPointSize 176,MaxPrimitiveCount 180,MaxVertexIndex 184,MaxStreams 188,MaxStreamStride 192,VertexShaderVersion 196,MaxVertexShaderConst 200,PixelShaderVersion 204,PixelShader1xMaxValue 208,DevCaps2 212,MaxNpatchTessellationLevel 216,Reserved5 220,MasterAdapterOrdinal 224,AdapterOrdinalInGroup 228,NumberOfAdaptersInGroup 232,DeclTypes 236,NumSimultaneousRTs 240,StretchRectFilterCaps 244,VS20Caps 248,PS20Caps 268,VertexTextureFilterCaps 288,MaxVShaderInstructionsExecuted 292,MaxPShaderInstructionsExecuted 296,MaxVertexShader30InstructionSlots 300,MaxPixelShader30InstructionSlots 304'.split(','))]


def caps_reads():
    """D3DCAPS9 fields Halo reads from its global copy, from the audit's objdump listing
    (absolute-address operands only; the copy handed to the config.txt parser at 0x580ace
    is read through a pointer and is not covered)."""
    import re
    uses = collections.defaultdict(list)
    for line in (A / 'text.objdump').open():
        for m in re.finditer(r'0x(75c[0-9a-f]{3})\b', line):
            a = int(m.group(1), 16)
            if CAPS_GLOBAL <= a < CAPS_GLOBAL + 304 and ':' in line:
                uses[a - CAPS_GLOBAL].append(' '.join(line.split(':', 1)[1].split()[:3]))
    rows = []
    for off in sorted(uses):
        name, base = max((f for f in CAPS_FIELDS if f[1] <= off), key=lambda f: f[1])
        rows.append((name + (f' (+{off - base})' if off != base else ''), len(uses[off]), uses[off][:2]))
    return rows


def main():
    sites = scan()
    out = {}
    lines = ['# Direct3D 9 call inventory (static)', '',
             'Generated by ~scripts/com-inventory.py~ from the accepted ~haloce.exe~. It counts calls of the form',
             '~reg = [global]; vt = [reg]; call [vt + offset]~ in straight-line code, so it is a **lower bound**:',
             'calls through device pointers passed in registers, and all texture/surface/buffer methods (objects held',
             'in structures) are not seen. Runtime traces will complete it. The device pointer is the global',
             '~0x6b840c~; ~IDirect3D9~ is ~0x6bd168~ (created by ~Direct3DCreate9(0x1f)~ at ~0x544a08~).', '']
    for g, (iface, names) in INTERFACES.items():
        rows = sorted(((o, len(a), a) for (gg, o), a in sites.items() if gg == g), key=lambda r: r[0])
        out[iface] = {names[o // 4] if o // 4 < len(names) else hex(o): {'offset': hex(o), 'calls': n, 'sites': [hex(x) for x in a]}
                      for o, n, a in rows}
        lines += [f'## {iface} (global ~{g:#x}~): {sum(n for _, n, _ in rows):,} call sites, {len(rows)} methods', '',
                  '| Method | Index | Call sites |', '|---|---|---|']
        lines += [f'| ~{names[o // 4]}~ | {o // 4} | {n} |' for o, n, _ in sorted(rows, key=lambda r: -r[1])]
        lines.append('')
    other = collections.Counter()
    for (g, o), a in sites.items():
        if g not in INTERFACES:
            other[g] += len(a)
    lines += ['## Other interface pointers in globals', '',
              'DirectInput 8 and DirectSound 8 objects are created through ~DirectInput8Create~ and ~DirectSoundCreate8~,',
              'which Halo loads dynamically at ~0x54436c~ and ~0x544353~. These globals are not typed yet:', '',
              '| Global | Call sites |', '|---|---|'] + [f'| ~{g:#x}~ | {n} |' for g, n in other.most_common()]
    lines += ['', '## Device capabilities Halo reads', '',
              'Halo copies ~D3DCAPS9~ to ~0x75c420~ (~IDirect3D9::GetDeviceCaps~ at ~0x51a414~) and reads these fields by',
              'absolute address. They decide Halo\'s rendering path, so every value HaloPad reports for them needs a',
              'documented source.', '', '| Field | Reads | Examples |', '|---|---|---|']
    lines += [f'| ~{n}~ | {k} | ' + '; '.join(f'~{e}~' for e in ex) + ' |' for n, k, ex in caps_reads()]
    lines += ['', '## Shaders', '',
              'Halo ships encrypted effect collections (~shaders/EffectCollection_ps_1_1.enc~, ~_ps_1_4~, ~_ps_2_0~, ~vsh.enc~).',
              'Halo decrypts them itself and passes plain Direct3D shader bytecode to ~CreatePixelShader~ and',
              '~CreateVertexShader~, one call site each, so the Metal layer translates bytecode, not files.']
    (ROOT / 'docs' / 'D3D9-INVENTORY.md').write_text('\n'.join(lines).replace('~', chr(96)) + '\n')
    (A / 'com-inventory.json').write_text(json.dumps(out, indent=1) + '\n')
    print('\n'.join(lines[:40]).replace('~', chr(96)))


if __name__ == '__main__':
    main()
