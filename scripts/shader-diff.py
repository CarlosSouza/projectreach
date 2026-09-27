#!/usr/bin/env python3
"""Differential test for HaloPad's shader translator (G4).

Every program in Halo's shader collections (generated/analysis/shaders/) runs on Metal
through the translator (tools/shader_run.m, compute-kernel test mode) and in this file's
own reference interpreter, written from the Direct3D 9 shader rules independently of the
C translator. Inputs and constants are random; textures are ramps that bilinear filtering
reproduces exactly (2D, volume) or one colour per face (cube), rotated per stage so a
wrong stage is visible. Outputs must agree within float tolerance.

Usage: .venv/bin/python scripts/shader-diff.py [--cases N] [--only SUBSTRING] [--seed S]
Writes docs/artifacts/<date>/G4/shader-diff-<stamp>/result.json.
"""
import argparse
import datetime
import json
import pathlib
import shutil
import struct
import subprocess
import sys

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[1]
S = ROOT / 'generated' / 'analysis' / 'shaders'
RUNNER = ROOT / 'generated' / 'tools' / 'shader-run'
F = np.float32


# ---------------------------------------------------------------- bytecode

def programs():
    out = []
    d = (S / 'vsh.bin').read_bytes()
    o, i = 0, 0
    while o + 4 <= len(d):
        n, = struct.unpack_from('<I', d, o)
        if not n or o + 4 + n > len(d):
            break
        out.append((f'vsh#{i}', list(struct.unpack_from(f'<{n // 4}I', d, o + 4))))
        o += 4 + n
        i += 1
    for f in sorted(S.glob('EffectCollection_*.bin')):
        d = f.read_bytes()
        o = 0
        ne, = struct.unpack_from('<I', d, o); o += 4
        for _ in range(ne):
            ln, = struct.unpack_from('<I', d, o); o += 4
            name = d[o:o + ln].decode(); o += ln
            ns, = struct.unpack_from('<I', d, o); o += 4
            for k in range(ns):
                ln, = struct.unpack_from('<I', d, o); o += 4 + ln
                sz, = struct.unpack_from('<I', d, o); o += 4
                out.append((f'{f.stem}:{name}#{k}', list(struct.unpack_from(f'<{sz}I', d, o))))
                o += 4 * sz
    return out


def rtype(p):
    return ((p >> 28) & 7) | ((p >> 8) & 0x18)


def decode(tokens):
    """[(opcode, instruction token, [parameter tokens])] up to END."""
    ver = tokens[0]
    major = (ver >> 8) & 0xFF
    out, i = [], 1
    while tokens[i] != 0xFFFF:
        t = tokens[i]
        op = t & 0xFFFF
        if op == 0xFFFE:
            i += 1 + ((t >> 16) & 0x7FFF)
            continue
        if major >= 2:
            n = (t >> 24) & 0xF
        elif op == 81:
            n = 5
        else:
            n = 0
            while tokens[i + 1 + n] & 0x80000000:
                n += 1
        out.append((op, t, tokens[i + 1:i + 1 + n]))
        i += 1 + n
    return out


# ---------------------------------------------------------------- textures

def texture(dim, stage):
    """(dim, w, h, d, uint8 array [faces|depth][h][w][4])."""
    rot = stage % 4
    if dim == 2:
        w = h = 16
        y, x = np.mgrid[0:h, 0:w].astype(np.float64)
        ch = [(x + 0.5) / w, (y + 0.5) / h, x * y / 225.0, 1.0 - x / 30.0 - y / 30.0]
        a = np.stack([ch[(k + rot) % 4] for k in range(4)], -1)
        return dim, w, h, 1, np.round(a * 255).astype(np.uint8)[None]
    if dim == 4:
        w = h = d = 8
        z, y, x = np.mgrid[0:d, 0:h, 0:w].astype(np.float64)
        ch = [x / 7.0, y / 7.0, z / 7.0, 1.0 - (x + y + z) / 21.0]
        a = np.stack([ch[(k + rot) % 4] for k in range(4)], -1)
        return dim, w, h, d, np.round(a * 255).astype(np.uint8)
    # cube: every texel encodes its own direction (continuous across face edges, so seamless
    # filtering only changes in-between values); the reference evaluates the same function
    w = h = 64
    a = np.zeros((6, h, w, 4), np.uint8)
    y, x = np.mgrid[0:h, 0:w].astype(np.float64)
    s, tt = (x + 0.5) / w * 2 - 1, (y + 0.5) / h * 2 - 1
    for f in range(6):
        dvec = cube_dir(f, s, tt)
        a[f] = np.round(np.stack([cube_color(dvec, k, rot) for k in range(4)], -1) * 255).astype(np.uint8)
    return dim, w, h, 1, a, rot


def cube_dir(face, s, t):
    """Direction for face coordinates s, t in [-1, 1] (Direct3D/OpenGL cube face layout)."""
    one = np.ones_like(s)
    x, y, z = {0: (one, -t, -s), 1: (-one, -t, s), 2: (s, one, t), 3: (s, -one, -t), 4: (s, -t, one), 5: (-s, -t, -one)}[face]
    n = np.sqrt(x * x + y * y + z * z)
    return np.stack([x / n, y / n, z / n], -1)


def cube_color(dvec, k, rot):
    ch = [0.5 + 0.5 * dvec[..., 0], 0.5 + 0.5 * dvec[..., 1], 0.5 + 0.5 * dvec[..., 2], 0.75 + 0.25 * dvec[..., 0] * dvec[..., 1]]
    return ch[(k + rot) % 4]


def bilinear(img, u, v):
    """img [h][w][4] floats; u, v arrays -> (N, 4); clamp to edge; texel centres at +0.5."""
    h, w = img.shape[:2]
    x = u.astype(np.float64) * w - 0.5
    y = v.astype(np.float64) * h - 0.5
    x0, y0 = np.floor(x), np.floor(y)
    fx, fy = (x - x0)[:, None], (y - y0)[:, None]
    xi = [np.clip(x0, 0, w - 1).astype(int), np.clip(x0 + 1, 0, w - 1).astype(int)]
    yi = [np.clip(y0, 0, h - 1).astype(int), np.clip(y0 + 1, 0, h - 1).astype(int)]
    t = lambda a, b: img[yi[b], xi[a]]
    return (t(0, 0) * (1 - fx) * (1 - fy) + t(1, 0) * fx * (1 - fy) + t(0, 1) * (1 - fx) * fy + t(1, 1) * fx * fy)


def sample(tex, coord):
    dim, w, h, d, texels = tex[:5]
    img = texels.astype(np.float64) / 255.0
    if dim == 2:
        return bilinear(img[0], coord[:, 0], coord[:, 1]).astype(F)
    if dim == 4:
        z = coord[:, 2].astype(np.float64) * d - 0.5
        z0 = np.floor(z)
        fz = (z - z0)[:, None]
        za, zb = np.clip(z0, 0, d - 1).astype(int), np.clip(z0 + 1, 0, d - 1).astype(int)
        out = np.zeros((len(coord), 4))
        for n in range(len(coord)):
            a = bilinear(img[za[n]], coord[n:n + 1, 0], coord[n:n + 1, 1])
            b = bilinear(img[zb[n]], coord[n:n + 1, 0], coord[n:n + 1, 1])
            out[n] = a * (1 - fz[n]) + b * fz[n]
        return out.astype(F)
    v = coord[:, :3].astype(np.float64)
    n = np.sqrt((v * v).sum(1, keepdims=True))
    dvec = v / np.where(n == 0, 1, n)
    rot = tex[5] if len(tex) > 5 else 0
    return np.stack([cube_color(dvec, k, rot) for k in range(4)], -1).astype(F)


# ---------------------------------------------------------------- reference interpreter

SWZ = lambda p: [(p >> (16 + 2 * k)) & 3 for k in range(4)]


class Ref:
    def __init__(self, tokens, n, ins, consts, textures):
        self.v = tokens[0]
        self.vs = self.v >> 16 == 0xFFFE
        self.major, self.minor = (self.v >> 8) & 0xFF, self.v & 0xFF
        self.ps1 = not self.vs and self.major == 1
        self.n, self.tex = n, textures
        self.r = {}
        self.defs = {}
        self.c = consts['c']
        self.a0 = np.zeros((n, 4), np.int64)
        if self.vs:
            self.vin = ins
            self.o = {'pos': np.zeros((n, 4), F), 'c0': np.zeros((n, 4), F), 'c1': np.zeros((n, 4), F),
                      'fog': np.ones(n, F), 'psize': np.ones(n, F)}
            for k in range(8):
                self.o[f't{k}'] = np.zeros((n, 4), F)
        else:
            self.col = [np.clip(ins[:, 0], 0, 1), np.clip(ins[:, 1], 0, 1)]
            self.tc = [ins[:, 2 + k] for k in range(8)]
            self.t = {}
            self.tm = {}
            self.oc0 = np.zeros((n, 4), F)
            self.killed = np.zeros(n, bool)

    def reg(self, p, rel):
        t, num = rtype(p), p & 0x7FF
        if t == 0:
            return self.r.get(num, np.zeros((self.n, 4), F))
        if t == 1:
            return self.vin[:, num] if self.vs else self.col[num]
        if t == 2:
            if p & 0x2000:
                comp = ((rel >> 16) & 3) if rel is not None else 0
                idx = self.a0[:, comp] + num
                ok = (idx >= 0) & (idx < 256)
                return np.where(ok[:, None], self.c[np.clip(idx, 0, 255)], F(0)).astype(F)
            v = self.defs.get(num)
            if v is None:
                v = np.broadcast_to(self.c[num], (self.n, 4))
            return np.clip(v, -1, 1) if self.ps1 else v
        if t == 3:
            if self.vs:
                return self.a0.astype(F)
            if self.ps1 and self.minor < 4:
                return self.t.get(num, np.zeros((self.n, 4), F))
            return self.tc[num]
        raise ValueError(f'source register type {t}')

    def src(self, p, rel=None):
        v = self.reg(p, rel)[:, SWZ(p)]
        m = (p >> 24) & 0xF
        f = {0: lambda x: x, 1: lambda x: -x, 2: lambda x: x - F(0.5), 3: lambda x: -(x - F(0.5)), 4: lambda x: F(2) * x - F(1),
             5: lambda x: -(F(2) * x - F(1)), 6: lambda x: F(1) - x, 7: lambda x: F(2) * x, 8: lambda x: F(-2) * x,
             9: lambda x: x / x[:, 2:3], 10: lambda x: x / x[:, 3:4], 11: np.abs, 12: lambda x: -np.abs(x)}[m]
        return f(v).astype(F)

    def scalar(self, p, v):
        return v[:, 3] if ((p >> 16) & 0xFF) == 0xE4 else v[:, 0]

    def write(self, p, val):
        val = np.broadcast_to(val, (self.n, 4)).astype(F)
        shift = (p >> 24) & 0xF
        if shift:
            val = val * F({1: 2, 2: 4, 3: 8, 13: 0.125, 14: 0.25, 15: 0.5}[shift])
        if (p >> 20) & 1:
            val = np.clip(val, 0, 1)
        elif self.ps1:
            val = np.clip(val, -8, 8)
        mask = [(p >> (16 + k)) & 1 for k in range(4)]
        t, num = rtype(p), p & 0x7FF
        if t == 0:
            cur = self.r.setdefault(num, np.zeros((self.n, 4), F))
        elif t == 3 and not self.vs:
            cur = self.t.setdefault(num, np.zeros((self.n, 4), F))
        elif t == 4:
            if num == 0:
                cur = self.o['pos']
            else:
                self.o['fog' if num == 1 else 'psize'] = val[:, 0].copy()
                return
        elif t == 5:
            cur = self.o[f'c{num}']
            val = np.clip(val, 0, 1)
        elif t == 6:
            cur = self.o[f't{num}']
        elif t == 8:
            cur = self.oc0
        else:
            raise ValueError(f'destination register type {t}')
        for k in range(4):
            if mask[k]:
                cur[:, k] = val[:, k]

    def run(self, insns):
        for op, tok, p in insns:
            if op == 81:
                self.defs[p[0] & 0x7FF] = np.broadcast_to(np.array(struct.unpack('<4f', struct.pack('<4I', *p[1:5])), F), (self.n, 4))
        pending = None
        for i, (op, tok, p) in enumerate(insns):
            if op in (31, 81, 0xFFFD):
                continue
            val = self.op(op, tok, p)
            co = self.ps1 and bool(tok & 0x40000000)
            if pending is not None and co and val is not None:
                self.write(pending[0], pending[1])
                self.write(p[0], val)
                pending = None
                continue
            if pending is not None:
                self.write(*pending)
                pending = None
            if val is None:
                continue
            nxt = insns[i + 1] if i + 1 < len(insns) else None
            if self.ps1 and nxt and nxt[1] & 0x40000000:
                pending = (p[0], val)
            else:
                self.write(p[0], val)
        if pending is not None:
            self.write(*pending)

    def sources(self, p):
        out, k = [], 1
        while k < len(p):
            rel = None
            if self.major >= 2 and p[k] & 0x2000 and k + 1 < len(p):
                rel = p[k + 1]
            out.append((p[k], rel))
            k += 2 if rel is not None else 1
        return out

    def op(self, op, tok, p):
        srcs = self.sources(p)
        S = lambda k: self.src(*srcs[k])
        rep = lambda x: np.repeat(x[:, None], 4, 1).astype(F)
        with np.errstate(all='ignore'):
            if op == 0:
                return None
            if op == 1:
                if self.vs and rtype(p[0]) == 3:
                    self.a0 = np.floor(S(0)).astype(np.int64)
                    return None
                return S(0)
            if op == 46:
                self.a0 = np.rint(S(0)).astype(np.int64)
                return None
            if op == 2: return S(0) + S(1)
            if op == 3: return S(0) - S(1)
            if op == 4: return S(0) * S(1) + S(2)
            if op == 5: return S(0) * S(1)
            if op == 6: return rep(F(1) / self.scalar(srcs[0][0], S(0)))
            if op == 7: return rep(F(1) / np.sqrt(np.abs(self.scalar(srcs[0][0], S(0)))))
            if op == 8: return rep((S(0)[:, :3] * S(1)[:, :3]).sum(1))
            if op == 9: return rep((S(0) * S(1)).sum(1))
            if op == 10: return np.minimum(S(0), S(1))
            if op == 11: return np.maximum(S(0), S(1))
            if op == 12: return (S(0) < S(1)).astype(F)
            if op == 13: return (S(0) >= S(1)).astype(F)
            if op == 14: return rep(np.exp2(self.scalar(srcs[0][0], S(0))))
            if op == 15: return rep(np.log2(np.abs(self.scalar(srcs[0][0], S(0)))))
            if op == 16:
                s = S(0)
                d = np.zeros((self.n, 4), F); d[:, 0] = 1; d[:, 3] = 1
                pos = s[:, 0] > 0
                d[:, 1] = np.where(pos, s[:, 0], 0)
                d[:, 2] = np.where(pos & (s[:, 1] > 0), np.power(np.maximum(s[:, 1], 0), np.clip(s[:, 3], -128, 128)), 0)
                return d
            if op == 17:
                a, b = S(0), S(1)
                return np.stack([np.ones(self.n, F), a[:, 1] * b[:, 1], a[:, 2], b[:, 3]], 1)
            if op == 18: return S(0) * (S(1) - S(2)) + S(2)
            if op == 19: return S(0) - np.floor(S(0))
            if op in (20, 21, 22, 23, 24):
                rows = {20: 4, 21: 3, 22: 4, 23: 3, 24: 2}[op]
                n3 = 3 if op >= 22 else 4
                a = S(0)
                out = np.zeros((self.n, 4), F)
                for r in range(rows):
                    b = self.src(srcs[1][0] + r, srcs[1][1])
                    out[:, r] = (a[:, :n3] * b[:, :n3]).sum(1)
                return out
            if op == 32: return rep(np.power(np.abs(self.scalar(srcs[0][0], S(0))), self.scalar(srcs[1][0], S(1))))
            if op == 35: return np.abs(S(0))
            if op == 80:
                if self.ps1 and self.minor < 4:
                    return np.where((self.r.get(0, np.zeros((self.n, 4), F))[:, 3] > 0.5)[:, None], S(1), S(2))
                return np.where(S(0) > 0.5, S(1), S(2))
            if op == 88: return np.where(S(0) >= 0, S(1), S(2))
            if op == 90: return rep((S(0)[:, :2] * S(1)[:, :2]).sum(1) + S(2)[:, 0])
            if op == 65:
                num = p[0] & 0x7FF
                v = self.tc[num] if rtype(p[0]) == 3 else self.r.get(num, np.zeros((self.n, 4), F))
                self.killed |= (v[:, :3] < 0).any(1)
                return None
            if op == 64:
                if self.minor < 4:
                    tcv = self.tc[p[0] & 0x7FF]
                    return np.clip(np.concatenate([tcv[:, :3], np.ones((self.n, 1), F)], 1), 0, 1)
                return S(0)
            if op == 66:
                if self.ps1 and self.minor < 4:
                    num = p[0] & 0x7FF
                    return sample(self.tex[num], self.tc[num])
                if self.ps1:
                    return sample(self.tex[p[0] & 0x7FF], S(0))
                return sample(self.tex[srcs[1][0] & 0x7FF], S(0))
            if op in (71, 73):
                num = p[0] & 0x7FF
                self.tm[num] = (self.tc[num][:, :3] * S(0)[:, :3]).sum(1)
                return None
            if op == 72:
                num = p[0] & 0x7FF
                coord = np.stack([self.tm[num - 1], (self.tc[num][:, :3] * S(0)[:, :3]).sum(1), np.zeros(self.n), np.ones(self.n)], 1)
                return sample(self.tex[num], coord)
            if op in (74, 76, 77):
                num = p[0] & 0x7FF
                nrm = np.stack([self.tm[num - 2], self.tm[num - 1], (self.tc[num][:, :3] * S(0)[:, :3]).sum(1)], 1)
                if op == 74:
                    return sample(self.tex[num], nrm)
                e = np.stack([self.tc[num - 2][:, 3], self.tc[num - 1][:, 3], self.tc[num][:, 3]], 1) if op == 77 else S(1)[:, :3]
                r = 2 * (nrm * e).sum(1, keepdims=True) / (nrm * nrm).sum(1, keepdims=True) * nrm - e
                return sample(self.tex[num], r)
        raise ValueError(f'opcode {op}')

    def outputs(self, consts):
        if self.vs:
            o = self.o
            out = np.zeros((self.n, 12, 4), F)
            out[:, 0], out[:, 1], out[:, 2] = o['pos'], o['c0'], o['c1']
            for k in range(8):
                out[:, 3 + k] = o[f't{k}']
            out[:, 11, 0], out[:, 11, 1] = o['fog'], o['psize']
            return out
        color = np.clip(self.r.get(0, np.zeros((self.n, 4), F)), 0, 1) if self.ps1 else self.oc0
        out = np.zeros((self.n, 2, 4), F)
        out[:, 0] = color
        out[:, 1, 0] = self.killed
        return out


# ---------------------------------------------------------------- jobs

def sampler_dims(tokens, insns):
    dims = {}
    vs = tokens[0] >> 16 == 0xFFFE
    major, minor = (tokens[0] >> 8) & 0xFF, tokens[0] & 0xFF
    if vs:
        return dims
    for op, tok, p in insns:
        if op == 31 and len(p) >= 2 and rtype(p[1]) == 10:
            dims[p[1] & 0x7FF] = (p[0] >> 27) & 0xF
    for op, tok, p in insns:
        if op in (74, 76, 77):
            dims.setdefault(p[0] & 0x7FF, 3)
        if op in (66, 72, 74, 76, 77) and major == 1:
            dims.setdefault(p[0] & 0x7FF, 2)
        if op == 66 and major >= 2:
            dims.setdefault(p[2] & 0x7FF if len(p) > 2 else p[-1] & 0x7FF, 2)
    return dims


def make_job(tokens, n, rng):
    insns = decode(tokens)
    vs = tokens[0] >> 16 == 0xFFFE
    if vs:
        ins = rng.uniform(-2, 2, (n, 16, 4)).astype(F)
        c = rng.uniform(-2, 2, (256, 4)).astype(F)
        cblock = c.tobytes() + np.zeros((16, 4), np.int32).tobytes() + np.zeros(16, np.uint32).tobytes() + np.zeros(4, F).tobytes()
    else:
        ins = np.zeros((n, 11, 4), F)
        ins[:, 0:2] = rng.uniform(-0.1, 1.1, (n, 2, 4))
        ins[:, 2:10, :3] = rng.uniform(-0.25, 1.25, (n, 8, 3))
        ins[:, 2:10, 3] = rng.uniform(0.5, 1.5, (n, 8))
        ins[:, 10, 0] = rng.uniform(0, 1, n)
        c = rng.uniform(-1.5, 1.5, (224, 4)).astype(F)
        cblock = (c.tobytes() + np.zeros((16, 4), np.int32).tobytes() + np.zeros(16, np.uint32).tobytes() + np.zeros(4, F).tobytes()
                  + struct.pack('<IfII', 8, 0.0, 0, 0))                # alpha test always, no fog
    textures = {s: texture(d, s) for s, d in sampler_dims(tokens, insns).items()}
    job = struct.pack('<III', 0x4A535048, 0 if vs else 1, len(tokens)) + struct.pack(f'<{len(tokens)}I', *tokens)
    job += struct.pack('<II', n, 16 if vs else 11) + ins.tobytes() + struct.pack('<I', len(cblock)) + cblock
    for s in range(16):
        if s in textures:
            dim, w, h, d, texels = textures[s][:5]
            job += struct.pack('<IIII', dim, w, h, d) + texels.tobytes()
        else:
            job += struct.pack('<IIII', 0, 0, 0, 0)
    return job, insns, ins, {'c': c}, textures


def compare(ref, got, vs, cube=False):
    both_inf = np.isinf(ref) & np.isinf(got) & (np.sign(ref) == np.sign(got))
    both_nan = np.isnan(ref) & np.isnan(got)
    tol = (1.5e-2 if cube else 2e-3) + 2e-3 * np.abs(np.nan_to_num(ref, posinf=0, neginf=0))
    ok = both_inf | both_nan | (np.abs(np.nan_to_num(ref) - np.nan_to_num(got)) <= tol)
    if not vs:                                                     # colour ignored for killed pixels
        killed = ref[:, 1, 0] > 0.5
        ok[:, 0] |= killed[:, None]
    return ok


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--cases', type=int, default=256)
    ap.add_argument('--only')
    ap.add_argument('--seed', type=int, default=1)
    a = ap.parse_args()
    subprocess.run(['clang', '-O1', '-fobjc-arc', '-w', '-std=c2x', '-x', 'c', str(ROOT / 'port/runtime/halopad_shader.c'), '-x',
                    'objective-c', str(ROOT / 'tools/shader_run.m'), '-framework', 'Foundation', '-framework', 'Metal', '-o',
                    str(RUNNER)], check=True)
    work = ROOT / 'generated' / 'tools' / 'shader-diff'
    if work.exists():
        shutil.rmtree(work)
    work.mkdir(parents=True)
    rng = np.random.default_rng(a.seed)
    progs = [(name, t) for name, t in programs() if not a.only or a.only in name]
    jobs = {}
    for k, (name, toks) in enumerate(progs):
        job, insns, ins, consts, textures = make_job(toks, a.cases, rng)
        (work / f'{k:04d}.job').write_bytes(job)
        jobs[k] = (name, toks, insns, ins, consts, textures)
    r = subprocess.run([str(RUNNER), str(work)], capture_output=True, text=True)
    if r.stdout.strip():
        print(r.stdout.strip()[:3000])
    results, worst = {}, []
    for k, (name, toks, insns, ins, consts, textures) in jobs.items():
        outp = work / f'{k:04d}.out'
        if not outp.exists():
            results[name] = {'result': 'NO OUTPUT'}
            continue
        vs = toks[0] >> 16 == 0xFFFE
        got = np.frombuffer(outp.read_bytes(), F).reshape(a.cases, 12 if vs else 2, 4)
        ref = Ref(toks, a.cases, ins, consts, textures)
        try:
            ref.run(insns)
        except Exception as e:                                     # the reference itself refuses
            results[name] = {'result': f'REFERENCE: {e}'}
            continue
        exp = ref.outputs(consts)
        cube = any(t[0] == 3 for t in textures.values())
        ok = compare(exp, got, vs, cube)
        bad = int((~ok).any(axis=(1, 2)).sum())
        err = np.abs(np.nan_to_num(exp, posinf=0, neginf=0) - np.nan_to_num(got, posinf=0, neginf=0))
        if not vs:
            err[:, 0] = np.where((exp[:, 1, 0] > 0.5)[:, None], 0, err[:, 0])
        # PRECISION: rare, small differences in programs that sample textures (8-bit texels and
        # bilinear filtering precision amplified by bx2/x2/shift arithmetic); anything else fails
        samples = bool(textures)
        status = 'PASS' if bad == 0 else ('PRECISION' if samples and bad <= 0.02 * a.cases and float(err.max()) <= 0.035 else 'FAIL')
        results[name] = {'result': status, 'bad_cases': bad, 'cases': a.cases, 'cube': cube,
                         'max_error': float(err.max()), 'p99_error': float(np.percentile(err.max(axis=(1, 2)), 99))}
        if bad:
            idx = int(np.argmax((~ok).any(axis=(1, 2))))
            where = np.argwhere(~ok[idx])[0]
            worst.append((bad, name, idx, [int(x) for x in where], float(exp[idx][tuple(where)]), float(got[idx][tuple(where)])))
    npass = sum(1 for v in results.values() if v['result'] == 'PASS')
    nprec = sum(1 for v in results.values() if v['result'] == 'PRECISION')
    nfail = len(results) - npass - nprec
    print(f'{len(results)} programs, {a.cases} cases each: {npass} agree on every case, {nprec} within texture precision, {nfail} fail')
    for bad, name, idx, where, e, g in sorted(worst, reverse=True)[:25]:
        print(f'  {name}: {bad}/{a.cases} cases differ; first case {idx} output {where}: reference {e:.6g}, Metal {g:.6g}; '
              f'max error {results[name]["max_error"]:.4g}, 99th percentile {results[name]["p99_error"]:.4g}')
    other = [f'{n}: {v["result"]}' for n, v in results.items() if v['result'] not in ('PASS', 'FAIL')]
    for o in other[:10]:
        print('  ' + o)
    stamp = datetime.datetime.utcnow().strftime('%Y%m%dT%H%M%SZ')
    evid = ROOT / 'docs' / 'artifacts' / datetime.date.today().isoformat() / 'G4' / f'shader-diff-{stamp}'
    evid.mkdir(parents=True, exist_ok=True)
    (evid / 'result.json').write_text(json.dumps({'cases': a.cases, 'seed': a.seed, 'programs': results}, indent=1) + '\n')
    print('evidence', evid.relative_to(ROOT))
    return 0 if nfail == 0 else 1


if __name__ == '__main__':
    sys.exit(main())
