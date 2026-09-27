#!/usr/bin/env python3
"""HaloPad x86 differential oracle (reference tool; never linked into a HaloPad target).

Runs original functions from the accepted Custom Edition 1.10 haloce.exe inside the
Unicorn x86 emulator and records their observable behavior: return registers, flags,
stack effect, memory writes and requested output buffers.

Every static and delay import slot points at a trap. Reaching a trap stops the run
with the import's name unless the caller supplies an explicit handler, so a missing
dependency can never silently return a guessed value.

CLI:
  x86-oracle.py fixture crc32 [--cases N] [--seed S]   # G2b acceptance fixture
  x86-oracle.py run --spec spec.json [--out result.json]

Spec JSON: {"function": "0x59f2a2", "convention": "stdcall|cdecl|thiscall|fastcall",
            "args": [int | {"bytes": "<hex>"} | {"alloc": size}],
            "regs": {"ecx": int, ...}, "read": [{"arg": i, "size": n}]}
"""
import argparse
import datetime
import hashlib
import json
import os
import pathlib
import random
import struct
import sys
import zlib

import pefile
from unicorn import (Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE,
                     UC_HOOK_MEM_UNMAPPED, UC_HOOK_INSN, UC_PROT_ALL)
from unicorn import x86_const as X

ROOT = pathlib.Path(__file__).resolve().parents[1]
PROFILE = ROOT / 'config' / 'profiles' / 'custom-en-1.0.10.0621.json'

STACK_BASE, STACK_SIZE = 0x00100000, 0x000F0000     # below the 0x400000 image
HEAP_BASE, HEAP_SIZE = 0x10000000, 0x01000000
TRAP_BASE, TRAP_SIZE = 0x7FF00000, 0x00010000
TLS_BASE, TLS_SIZE = 0x7FFD0000, 0x00008000
TEB_BASE, TEB_SIZE = 0x7FFDE000, 0x00001000
GDT_BASE, GDT_SIZE = 0x7FFC0000, 0x00001000
STOP_ADDR = 0x7FFF0000
CODE_SELECTOR, DATA_SELECTOR, FS_SELECTOR = 1 << 3, 2 << 3, 3 << 3  # ring 0 flat code/data + TEB

REGS = {'eax': X.UC_X86_REG_EAX, 'ebx': X.UC_X86_REG_EBX, 'ecx': X.UC_X86_REG_ECX,
        'edx': X.UC_X86_REG_EDX, 'esi': X.UC_X86_REG_ESI, 'edi': X.UC_X86_REG_EDI,
        'ebp': X.UC_X86_REG_EBP, 'esp': X.UC_X86_REG_ESP}


class OracleError(RuntimeError):
    """A run could not be completed faithfully (trap, fault or timeout)."""


def sha256_file(path):
    return hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()


def _gdt_entry(base, limit, access, flags):
    e = limit & 0xFFFF
    e |= (base & 0xFFFFFF) << 16
    e |= (access & 0xFF) << 40
    e |= ((limit >> 16) & 0xF) << 48
    e |= (flags & 0xF) << 52
    e |= ((base >> 24) & 0xFF) << 56
    return struct.pack('<Q', e)


class Image:
    """The accepted executable, parsed once and verified against the profile."""

    def __init__(self, exe=None):
        profile = json.loads(PROFILE.read_text())
        self.path = pathlib.Path(exe) if exe else ROOT / profile['original_root'] / profile['executable']
        self.sha256 = sha256_file(self.path)
        if self.sha256 != profile['accepted_sha256']:
            raise OracleError(f'{self.path} does not match the accepted profile hash')
        self.pe = pefile.PE(str(self.path))
        self.base = self.pe.OPTIONAL_HEADER.ImageBase
        self.size = (self.pe.OPTIONAL_HEADER.SizeOfImage + 0xFFF) & ~0xFFF
        self.mapped = self.pe.get_memory_mapped_image()
        self.imports = []  # (slot VA, 'DLL!name', delay?)
        for kind, delay in (('DIRECTORY_ENTRY_IMPORT', False), ('DIRECTORY_ENTRY_DELAY_IMPORT', True)):
            for d in getattr(self.pe, kind, []):
                for imp in d.imports:
                    name = imp.name.decode() if imp.name else f'#{imp.ordinal}'
                    self.imports.append((imp.address, f'{d.dll.decode()}!{name}', delay))
        self.tls = None
        if hasattr(self.pe, 'DIRECTORY_ENTRY_TLS'):
            t = self.pe.DIRECTORY_ENTRY_TLS.struct
            self.tls = (t.StartAddressOfRawData, t.EndAddressOfRawData, t.AddressOfIndex, t.SizeOfZeroFill)


class Oracle:
    def __init__(self, image=None, import_handlers=None, max_instructions=50_000_000, cpuid=None, fpcw=None):
        """cpuid: optional callable(leaf, subleaf) -> (eax, ebx, ecx, edx). When set, every
        CPUID instruction returns these values instead of the emulator's own CPU model.
        fpcw: optional initial x87 control word (e.g. 0x007F = single precision, the mode
        Halo runs in under Direct3D 9; 0x037F = the power-on extended-precision default)."""
        self.image = image or Image()
        self.import_handlers = import_handlers or {}
        self.max_instructions = max_instructions
        self.cpuid = cpuid
        self.fpcw = fpcw
        self.cpuid_calls = []

    # -- machine setup -------------------------------------------------
    def _machine(self):
        img = self.image
        mu = Uc(UC_ARCH_X86, UC_MODE_32)
        mu.mem_map(img.base, img.size, UC_PROT_ALL)
        mu.mem_write(img.base, img.mapped[:img.size])
        mu.mem_map(STACK_BASE, STACK_SIZE)
        mu.mem_map(HEAP_BASE, HEAP_SIZE)
        mu.mem_map(TRAP_BASE, TRAP_SIZE)
        mu.mem_write(TRAP_BASE, b'\xCC' * TRAP_SIZE)
        mu.mem_map(STOP_ADDR, 0x1000)
        mu.mem_write(STOP_ADDR, b'\xF4')
        self.traps = {}
        for i, (slot, name, delay) in enumerate(img.imports):
            trap = TRAP_BASE + 16 * i
            self.traps[trap] = ('delay:' if delay else '') + name
            mu.mem_write(slot, struct.pack('<I', trap))
        # Thread environment: TEB via FS, TLS index 0 -> copy of the .tls template.
        mu.mem_map(TLS_BASE, TLS_SIZE)
        mu.mem_map(TEB_BASE, TEB_SIZE)
        tls_array, tls_block = TLS_BASE, TLS_BASE + 0x100
        if img.tls:
            start, end, index_va, zero_fill = img.tls
            template = bytes(mu.mem_read(start, end - start)) if end > start else b''
            mu.mem_write(tls_block, template + b'\0' * zero_fill)
            mu.mem_write(index_va, struct.pack('<I', 0))
        mu.mem_write(tls_array, struct.pack('<I', tls_block))
        teb = bytearray(0x100)
        struct.pack_into('<I', teb, 0x00, 0xFFFFFFFF)              # no SEH frame
        struct.pack_into('<I', teb, 0x04, STACK_BASE + STACK_SIZE)  # stack base
        struct.pack_into('<I', teb, 0x08, STACK_BASE)               # stack limit
        struct.pack_into('<I', teb, 0x18, TEB_BASE)                 # self
        struct.pack_into('<I', teb, 0x2C, tls_array)                # TLS array
        mu.mem_write(TEB_BASE, bytes(teb))
        mu.mem_map(GDT_BASE, GDT_SIZE)
        # Flat 32-bit code/data descriptors keep CS/SS/DS/ES 32-bit once GDTR is loaded.
        mu.mem_write(GDT_BASE + 1 * 8, _gdt_entry(0, 0xFFFFF, 0x9A, 0xC))
        mu.mem_write(GDT_BASE + 2 * 8, _gdt_entry(0, 0xFFFFF, 0x92, 0xC))
        mu.mem_write(GDT_BASE + 3 * 8, _gdt_entry(TEB_BASE, 0xFFF, 0x92, 0x4))
        mu.reg_write(X.UC_X86_REG_GDTR, (0, GDT_BASE, GDT_SIZE - 1, 0))
        for seg in (X.UC_X86_REG_SS, X.UC_X86_REG_DS, X.UC_X86_REG_ES, X.UC_X86_REG_GS):
            mu.reg_write(seg, DATA_SELECTOR)
        mu.reg_write(X.UC_X86_REG_CS, CODE_SELECTOR)
        mu.reg_write(X.UC_X86_REG_FS, FS_SELECTOR)
        if self.fpcw is not None:
            mu.reg_write(X.UC_X86_REG_FPCW, self.fpcw)
        return mu

    # -- one call ------------------------------------------------------
    def call(self, function, args=(), convention='stdcall', regs=None, reads=()):
        """Run one function. args: ints, bytes (copied to heap) or ('alloc', size).
        Returns a dict of observations; raises OracleError on trap/fault."""
        mu = self._machine()
        heap = HEAP_BASE
        values, buffers = [], []
        deferred = []
        for a in args:
            if isinstance(a, int):
                values.append(a & 0xFFFFFFFF); buffers.append(None)
            elif isinstance(a, tuple) and a and a[0] == 'ptr':
                # pointer into an earlier buffer argument: ('ptr', arg_index, byte_offset)
                values.append(None); buffers.append(None); deferred.append((len(values) - 1, a[1], a[2]))
            else:
                data = bytes(a) if isinstance(a, (bytes, bytearray)) else b'\0' * a[1]
                mu.mem_write(heap, data)
                values.append(heap); buffers.append((heap, len(data)))
                heap = (heap + len(data) + 0x40) & ~0xF
        for slot, target, offset in deferred:
            values[slot] = (values[target] + offset) & 0xFFFFFFFF
        regs = dict(regs or {})
        for name, v in list(regs.items()):
            if isinstance(v, tuple) and v and v[0] == 'ptr':
                regs[name] = (values[v[1]] + v[2]) & 0xFFFFFFFF
        stack_args = list(values)
        if convention == 'thiscall':
            regs.setdefault('ecx', stack_args.pop(0))
        elif convention == 'fastcall':
            for r in ('ecx', 'edx'):
                if stack_args:
                    regs.setdefault(r, stack_args.pop(0))
        elif convention not in ('stdcall', 'cdecl'):
            raise ValueError(convention)
        sp = STACK_BASE + STACK_SIZE - 0x1000
        frame = struct.pack('<I', STOP_ADDR) + b''.join(struct.pack('<I', v) for v in stack_args)
        mu.mem_write(sp, frame)
        for name, reg in REGS.items():
            if name != 'esp':
                mu.reg_write(reg, regs.get(name, 0))
        mu.reg_write(X.UC_X86_REG_ESP, sp)

        writes, fault = [], {}
        stack_lo, stack_hi = STACK_BASE, STACK_BASE + STACK_SIZE

        def on_trap(uc, address, size, _):
            name = self.traps.get(address, f'unknown trap {address:#x}')
            handler = self.import_handlers.get(name)
            if handler is None:
                fault['trap'] = name
                uc.emu_stop()
                return
            ret, pop = handler(uc)
            esp = uc.reg_read(X.UC_X86_REG_ESP)
            back = struct.unpack('<I', uc.mem_read(esp, 4))[0]
            uc.reg_write(X.UC_X86_REG_EAX, ret & 0xFFFFFFFF)
            uc.reg_write(X.UC_X86_REG_ESP, esp + 4 + pop)
            uc.reg_write(X.UC_X86_REG_EIP, back)

        def on_write(uc, access, address, size, value, _):
            if not (stack_lo <= address < stack_hi) and len(writes) < 200_000:
                writes.append((address, size, value & ((1 << (8 * size)) - 1)))

        def on_unmapped(uc, access, address, size, value, _):
            fault['unmapped'] = {'access': access, 'address': address, 'size': size,
                                 'eip': uc.reg_read(X.UC_X86_REG_EIP)}
            return False

        mu.hook_add(UC_HOOK_CODE, on_trap, begin=TRAP_BASE, end=TRAP_BASE + TRAP_SIZE - 1)
        mu.hook_add(UC_HOOK_MEM_WRITE, on_write)
        mu.hook_add(UC_HOOK_MEM_UNMAPPED, on_unmapped)
        if self.cpuid is not None:
            def on_cpuid(uc, _):
                leaf, sub = uc.reg_read(X.UC_X86_REG_EAX), uc.reg_read(X.UC_X86_REG_ECX)
                a, b, c, d = self.cpuid(leaf, sub)
                self.cpuid_calls.append(leaf)
                for reg, v in ((X.UC_X86_REG_EAX, a), (X.UC_X86_REG_EBX, b), (X.UC_X86_REG_ECX, c), (X.UC_X86_REG_EDX, d)):
                    uc.reg_write(reg, v & 0xFFFFFFFF)
                return 1  # skip the emulator's own CPUID
            mu.hook_add(UC_HOOK_INSN, on_cpuid, None, 1, 0, X.UC_X86_INS_CPUID)
        try:
            mu.emu_start(function, STOP_ADDR, count=self.max_instructions)
        except UcError as exc:
            fault.setdefault('error', str(exc))
        eip = mu.reg_read(X.UC_X86_REG_EIP)
        if fault:
            raise OracleError(json.dumps({'function': hex(function), 'eip': hex(eip), **fault}))
        if eip != STOP_ADDR:
            raise OracleError(f'{function:#x} did not return within {self.max_instructions} instructions (eip {eip:#x})')
        out = {name: mu.reg_read(reg) for name, reg in REGS.items()}
        out['eflags'] = mu.reg_read(X.UC_X86_REG_EFLAGS)
        out['stack_delta'] = out['esp'] - sp
        out['writes'] = writes
        out['buffers'] = {}
        for spec in reads:
            addr, size = buffers[spec['arg']]
            out['buffers'][spec['arg']] = bytes(mu.mem_read(addr, spec.get('size', size))).hex()
        out['arg_values'] = values
        st0 = mu.reg_read(X.UC_X86_REG_FP0)
        out['st0'] = list(st0) if isinstance(st0, tuple) else st0
        return out


# -- G2b acceptance fixture ---------------------------------------------
CRC32 = 0x59F2A2  # stdcall uint32 crc32(uint32 crc, const void *buf, uint32 len)


def fixture_crc32(cases=64, seed=0x4841_4C4F):
    oracle = Oracle()
    rng = random.Random(seed)
    lengths = list(range(0, 18)) + [4096, 255, 256, 257]
    while len(lengths) < cases:
        lengths.append(rng.randrange(0, 2048))
    results, failures = [], 0
    for i, n in enumerate(lengths[:cases]):
        data = bytes(rng.getrandbits(8) for _ in range(n))
        crc = rng.getrandbits(32) if i % 3 else 0
        obs = oracle.call(CRC32, [crc, data, n], 'stdcall')
        expected = zlib.crc32(data, crc) & 0xFFFFFFFF
        ok = obs['eax'] == expected and obs['stack_delta'] == 16 and not obs['writes']
        failures += not ok
        results.append({'len': n, 'crc_in': crc, 'eax': obs['eax'], 'expected': expected,
                        'stack_delta': obs['stack_delta'], 'nonstack_writes': len(obs['writes']),
                        'ok': ok})
    check = oracle.call(CRC32, [0, b'123456789', 9], 'stdcall')
    return {'fixture': 'crc32', 'function': hex(CRC32), 'image_sha256': oracle.image.sha256,
            'seed': seed, 'cases': len(results), 'failures': failures,
            'check_value': hex(check['eax']), 'check_ok': check['eax'] == 0xCBF43926,
            'results': results}


def evidence_dir(goal='G2b'):
    stamp = datetime.datetime.utcnow().strftime('%Y%m%dT%H%M%SZ')
    d = ROOT / 'docs' / 'artifacts' / datetime.date.today().isoformat() / goal / f'oracle-{stamp}-{os.getpid()}'
    d.mkdir(parents=True, exist_ok=True)
    return d


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest='cmd', required=True)
    fx = sub.add_parser('fixture')
    fx.add_argument('name', choices=['crc32'])
    fx.add_argument('--cases', type=int, default=64)
    fx.add_argument('--seed', type=int, default=0x48414C4F)
    rn = sub.add_parser('run')
    rn.add_argument('--spec', required=True, type=pathlib.Path)
    rn.add_argument('--out', type=pathlib.Path)
    a = ap.parse_args()

    if a.cmd == 'fixture':
        report = fixture_crc32(a.cases, a.seed)
        d = evidence_dir()
        (d / 'crc32.json').write_text(json.dumps(report, indent=2) + '\n')
        ok = report['failures'] == 0 and report['check_ok']
        print(f"{'PASS' if ok else 'FAIL'} crc32 {report['function']}: {report['cases'] - report['failures']}/{report['cases']} cases, "
              f"check {report['check_value']}; evidence {d.relative_to(ROOT)}")
        return 0 if ok else 1

    spec = json.loads(a.spec.read_text())
    args = []
    for x in spec.get('args', []):
        if isinstance(x, int):
            args.append(x)
        elif 'bytes' in x:
            args.append(bytes.fromhex(x['bytes']))
        else:
            args.append(('alloc', x['alloc']))
    obs = Oracle().call(int(spec['function'], 0), args, spec.get('convention', 'stdcall'),
                        spec.get('regs'), spec.get('read', []))
    obs['writes'] = [[hex(w[0]), w[1], w[2]] for w in obs['writes']]
    text = json.dumps(obs, indent=2) + '\n'
    (a.out.write_text(text) if a.out else sys.stdout.write(text))
    return 0


if __name__ == '__main__':
    sys.exit(main())
