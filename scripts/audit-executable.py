#!/usr/bin/env python3
"""G2a executable audit for the accepted Custom Edition 1.10 haloce.exe.

Recursive-descent disassembly (capstone) from the entry point, TLS callbacks and
code addresses found in code/data; jump-table decoding; indirect branch inventory;
prologue-based discovery in remaining gaps; relocation recovery for the stripped
image; dynamic module / GetProcAddress inventory; instruction-family statistics.

Outputs:
  generated/analysis/<profile>/relocations.csv   (SRW format: fixup,target[,i])
  docs/artifacts/<date>/G2a/audit-<stamp>/audit.json   (private full manifest)
  docs/EXECUTION-MODEL.md                              (summary; no game bytes)
"""
import argparse
import bisect
import collections
import datetime
import hashlib
import json
import pathlib
import re
import struct
import sys

import capstone
import pefile
from capstone import x86_const as X

ROOT = pathlib.Path(__file__).resolve().parents[1]
PROFILE_ID = 'custom-en-1.0.10.0621'

NORETURN_IMPORTS = {'ExitProcess', 'ExitThread', 'FatalAppExitA', 'FatalAppExitW', 'RaiseException',
                    '_CxxThrowException', 'abort', 'exit', '_exit', 'TerminateProcess'}
PADDING = {0xCC, 0x90}
PROLOGUES = [bytes.fromhex(h) for h in (
    '558bec', '538b', '568b', '578b', '83ec', '81ec', '6aff68', '64a100000000', '5156', '5153', '5355',
    '5657', '8b442404', '8b4c2404', '8b542404', 'a1', '8b0d', '8b15', '33c0', 'b8')]
UNCOND = {'jmp', 'ljmp'}
ENDS = {'ret', 'retf', 'int3', 'hlt', 'ud2', 'iretd'}
ARITH = {'cmp', 'add', 'sub', 'and', 'or', 'xor', 'imul', 'test', 'adc', 'sbb', 'shl', 'shr', 'sar'}
# Compiler alignment fillers that precede labels reached only indirectly.
FILLERS = [bytes.fromhex(h) for h in ('8da42400000000', '8d9b00000000', '8da40000000000', '8d642400', '8d4900', '8d4000', '8bff', '8bc0', '8bc9', '8bd2', '8bdb', '8bf6', '90')]
# Instructions that essentially never occur in this compiler's code; seeing one while
# test-decoding means the bytes are data.
JUNK = {'in', 'out', 'insb', 'insd', 'outsb', 'outsd', 'arpl', 'bound', 'les', 'lds', 'into', 'aaa', 'aas',
        'daa', 'das', 'aam', 'aad', 'salc', 'hlt', 'iretd', 'retf', 'lcall', 'ljmp', 'sti', 'cli', 'wait',
        'enter', 'pushal', 'popal', 'pushfd', 'sahf', 'lahf', 'xlatb', 'icebp', 'int1', 'fwait'}


def sha256(path):
    return hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()


class Audit:
    def __init__(self, exe):
        self.pe = pefile.PE(str(exe))
        self.img = self.pe.get_memory_mapped_image()
        self.base = self.pe.OPTIONAL_HEADER.ImageBase
        self.end = self.base + self.pe.OPTIONAL_HEADER.SizeOfImage
        self.sections = []
        for s in self.pe.sections:
            name = s.Name.rstrip(b'\0').decode()
            va = self.base + s.VirtualAddress
            self.sections.append({'name': name, 'va': va, 'vsize': s.Misc_VirtualSize,
                                  'raw': s.SizeOfRawData, 'characteristics': s.Characteristics})
        text = self.section('.text')
        self.text_lo, self.text_hi = text['va'], text['va'] + text['vsize']
        self.md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        self.md.detail = True
        self.imports = {}
        for kind, delay in (('DIRECTORY_ENTRY_IMPORT', False), ('DIRECTORY_ENTRY_DELAY_IMPORT', True)):
            for d in getattr(self.pe, kind, []):
                for imp in d.imports:
                    name = imp.name.decode() if imp.name else f'#{imp.ordinal}'
                    self.imports[imp.address] = {'dll': d.dll.decode(), 'name': name, 'delay': delay}
        self.insn = {}            # addr -> (size, mnemonic)
        self.owner = {}           # byte addr -> insn start (instruction bodies only)
        self.functions = {}       # addr -> source
        self.blocks = set()
        self.jump_tables = []
        self.index_tables = []
        self.indirect = []
        self.bad = []
        self.overlaps = []
        self.relocs = {}          # fixup -> (target, kind)
        self.uncertain = {}
        self.data_code_ptrs = {}
        self.getproc = []
        self.stats = collections.Counter()
        self.mnemonics = collections.Counter()
        self.table_bytes = set()
        self.tls_callbacks = []
        self.heuristic_functions = 0
        self.data_in_text = {}    # target -> first referrer (rejected by probe)
        self.probe_rejects = collections.Counter()

    def section(self, name):
        return next(s for s in self.sections if s['name'] == name)

    def section_of(self, va):
        for s in self.sections:
            if s['va'] <= va < s['va'] + max(s['vsize'], s['raw']):
                return s['name']
        return None

    def in_image(self, v):
        return self.base <= v < self.end

    def in_text(self, v):
        return self.text_lo <= v < self.text_hi

    def dword(self, va):
        return struct.unpack_from('<I', self.img, va - self.base)[0]

    def decode(self, addr):
        o = addr - self.base
        for ins in self.md.disasm(self.img[o:o + 16], addr, 1):
            return ins
        return None

    def add_reloc(self, fixup, target, kind, certain=True):
        if certain and self.section_of(target) is None:
            # Points into the PE header. Code references (CRT MZ/__ImageBase checks) stay
            # literal: the guest image keeps its original addresses and the header page is
            # mapped at the image base (G2e). SRW's llasm backend cannot express image-base
            # fixups (it indexes section[ImageBase]). Data dwords here are integers.
            kind, certain = (kind + ':imagebase-literal') if kind.startswith('code') else 'data-ptr:header', False
        (self.relocs if certain else self.uncertain)[fixup] = (target, kind)

    def verify_relocs(self):
        """Mirror SRW_loader.c LoadRelocations: value must match, target must resolve."""
        problems = []
        for fixup, (target, kind) in self.relocs.items():
            if self.section_of(fixup) is None:
                problems.append(f'fixup {fixup:#x} outside sections')
            if self.dword(fixup) != target:
                problems.append(f'fixup {fixup:#x} value mismatch')
            if self.section_of(target) is None and not kind.endswith(':imagebase'):
                problems.append(f'target {target:#x} of {fixup:#x} outside sections')
        return problems

    def demote_mid_instruction_targets(self):
        """A value that points into the middle of a decoded instruction is not an
        address (for example the ASCII tag 'FPS' = 0x00535046). Demote it."""
        demoted = 0
        for fixup, (target, kind) in list(self.relocs.items()):
            if self.in_text(target) and target in self.owner:
                del self.relocs[fixup]
                self.uncertain[fixup] = (target, kind + ':mid-instruction')
                demoted += 1
        return demoted

    def probe(self, start, limit=20000):
        """Side-effect-free test decode of the code reachable from start (intra-procedural)."""
        seen, work = {}, [start]
        while work:
            a = work.pop()
            while True:
                if not self.in_text(a):
                    return 'leaves .text'
                if a in self.insn or a in seen:
                    break
                if a in self.owner or a in self.table_bytes:
                    return 'lands inside known code'
                ins = self.decode(a)
                if ins is None:
                    return 'undecodable'
                if any((a + k) in self.insn or (a + k) in self.owner or (a + k) in self.table_bytes
                       for k in range(1, ins.size)):
                    return 'overlaps known code'
                o = a - self.base
                if ins.mnemonic in JUNK or self.img[o:o + 2] == b'\0\0':
                    return 'data-like instruction'
                seen[a] = ins.size
                if len(seen) > limit:
                    return 'too long'
                m = ins.mnemonic
                if m in ENDS or m.startswith('ret'):
                    break
                op = ins.operands[0] if ins.operands else None
                if m in UNCOND:
                    if op is not None and op.type == X.X86_OP_IMM:
                        work.append(op.imm & 0xFFFFFFFF)
                    break
                if capstone.CS_GRP_JUMP in ins.groups and op is not None and op.type == X.X86_OP_IMM:
                    work.append(op.imm & 0xFFFFFFFF)
                a += ins.size
        starts = sorted(seen)
        for x, y in zip(starts, starts[1:]):
            if x + seen[x] > y:
                return 'self-overlapping'
        return None

    def try_entry(self, target, source):
        if target in self.insn:
            return True
        reason = self.probe(target)
        if reason:
            self.probe_rejects[reason] += 1
            self.data_in_text.setdefault(target, source)
            return False
        self.functions.setdefault(target, source)
        self.trace(target, source)
        return True

    def trace(self, start, source):
        work = [(start, source)]
        while work:
            addr, src = work.pop()
            while True:
                if not self.in_text(addr):
                    self.bad.append({'address': addr, 'from': src, 'reason': 'outside .text'})
                    break
                if addr in self.insn:
                    break
                if addr in self.owner or addr in self.table_bytes:
                    self.overlaps.append({'address': addr, 'from': src, 'inside': self.owner.get(addr, 'table')})
                    break
                ins = self.decode(addr)
                if ins is None:
                    self.bad.append({'address': addr, 'from': src, 'reason': 'undecodable'})
                    break
                if any((addr + k) in self.insn or (addr + k) in self.owner or (addr + k) in self.table_bytes
                       for k in range(1, ins.size)):
                    self.overlaps.append({'address': addr, 'from': src, 'inside': None})
                    break
                self.insn[addr] = (ins.size, ins.mnemonic)
                for k in range(1, ins.size):
                    self.owner[addr + k] = addr
                self.mnemonics[ins.mnemonic] += 1
                self.classify_groups(ins)
                code_refs = self.operand_refs(ins)
                m = ins.mnemonic
                ops = ins.operands
                nxt = addr + ins.size
                for t in code_refs:
                    if t not in self.functions and t not in self.data_in_text:
                        self.pending.append((t, f'code-imm@{addr:#x}'))
                if m in ENDS or m.startswith('ret'):
                    break
                if m == 'call':
                    op = ops[0]
                    if op.type == X.X86_OP_IMM:
                        t = op.imm & 0xFFFFFFFF
                        self.functions.setdefault(t, f'call@{addr:#x}')
                        work.append((t, f'call@{addr:#x}'))
                        if self.is_noreturn_thunk(t):
                            break
                    else:
                        kind = self.indirect_kind(ins)
                        self.indirect.append({'address': addr, 'type': 'call', 'kind': kind, 'op': ins.op_str})
                        if kind.startswith('import') and kind.split('!')[-1] in NORETURN_IMPORTS:
                            break
                        if kind.startswith('import') and kind.endswith('!GetProcAddress'):
                            self.note_getproc(addr)
                    addr = nxt
                    continue
                if m in UNCOND:
                    op = ops[0]
                    if op.type == X.X86_OP_IMM:
                        t = op.imm & 0xFFFFFFFF
                        self.blocks.add(t)
                        work.append((t, f'jmp@{addr:#x}'))
                    else:
                        targets = self.jump_table(ins)
                        if targets:
                            for t in targets:
                                self.blocks.add(t)
                                work.append((t, f'jtab@{addr:#x}'))
                        else:
                            kind = self.indirect_kind(ins)
                            self.indirect.append({'address': addr, 'type': 'jmp', 'kind': kind, 'op': ins.op_str})
                    break
                if capstone.CS_GRP_JUMP in ins.groups:
                    op = ops[0]
                    if op.type == X.X86_OP_IMM:
                        t = op.imm & 0xFFFFFFFF
                        self.blocks.add(t)
                        work.append((t, f'jcc@{addr:#x}'))
                addr = nxt

    def classify_groups(self, ins):
        g = set(ins.groups)
        if X.X86_GRP_FPU in g or ins.mnemonic.startswith('f'):
            self.stats['x87'] += 1
        if X.X86_GRP_MMX in g:
            self.stats['mmx'] += 1
        if g & {X.X86_GRP_SSE1, X.X86_GRP_SSE2, X.X86_GRP_SSE3, X.X86_GRP_SSSE3, X.X86_GRP_SSE41, X.X86_GRP_SSE42}:
            self.stats['sse'] += 1
        if X.X86_GRP_3DNOW in g:
            self.stats['3dnow'] += 1
        if ins.prefix[0] in (0xF2, 0xF3) and ins.mnemonic.startswith(('movs', 'stos', 'cmps', 'scas', 'lods', 'rep')):
            self.stats['rep_string'] += 1
        if ins.mnemonic in ('cpuid', 'rdtsc', 'int', 'into', 'in', 'out', 'cli', 'sti', 'cmpxchg', 'xadd'):
            self.stats['special:' + ins.mnemonic] += 1
        if ins.prefix[0] == 0xF0:
            self.stats['lock_prefix'] += 1
        if ins.prefix[1] == 0x64:
            self.stats['fs_segment'] += 1

    def operand_refs(self, ins):
        """Record relocations for absolute operands; return code addresses to trace."""
        code = []
        enc = ins.encoding
        m = ins.mnemonic
        branch = capstone.CS_GRP_JUMP in ins.groups or capstone.CS_GRP_CALL in ins.groups
        for op in ins.operands:
            if op.type == X.X86_OP_MEM and enc.disp_size == 4:
                v = op.mem.disp & 0xFFFFFFFF
                if self.in_image(v):
                    self.add_reloc(ins.address + enc.disp_offset, v, 'code-disp')
                    if self.in_text(v):
                        # A .text address used as a memory operand is read as data
                        # (switch index tables, constants); never probe it as code.
                        self.data_in_text.setdefault(v, f'memory-operand@{ins.address:#x}')
                    if self.in_text(v) and (op.mem.index != 0 or op.mem.base != 0) and m == 'movzx':
                        self.index_tables.append({'address': v, 'user': ins.address})
            elif op.type == X.X86_OP_IMM and enc.imm_size == 4 and not branch:
                v = op.imm & 0xFFFFFFFF
                if self.in_image(v):
                    if m in ARITH:
                        self.add_reloc(ins.address + enc.imm_offset, v, f'code-imm-{m}', certain=False)
                    else:
                        self.add_reloc(ins.address + enc.imm_offset, v, 'code-imm')
                        if self.in_text(v):
                            code.append(v)
        return code

    def indirect_kind(self, ins):
        op = ins.operands[0]
        if op.type == X.X86_OP_REG:
            return 'register'
        if op.type == X.X86_OP_MEM:
            disp = op.mem.disp & 0xFFFFFFFF
            if op.mem.base == 0 and op.mem.index == 0:
                if disp in self.imports:
                    i = self.imports[disp]
                    return ('import:delay:' if i['delay'] else 'import:') + f"{i['dll']}!{i['name']}"
                return 'absolute-pointer'
            if op.mem.index == 0:
                return 'vtable-or-struct'
            return 'indexed-memory'
        return 'other'

    def is_noreturn_thunk(self, t):
        ins = self.decode(t) if self.in_text(t) else None
        if ins and ins.mnemonic == 'jmp' and ins.operands[0].type == X.X86_OP_MEM:
            disp = ins.operands[0].mem.disp & 0xFFFFFFFF
            return self.imports.get(disp, {}).get('name') in NORETURN_IMPORTS
        return False

    def jump_table(self, ins):
        op = ins.operands[0]
        if op.type != X.X86_OP_MEM or op.mem.scale != 4 or op.mem.index == 0 or op.mem.base != 0:
            return None
        table = op.mem.disp & 0xFFFFFFFF
        if not self.in_image(table):
            return None
        bound = self.table_bound(ins.address, op.mem.index)
        targets = []
        va = table
        while len(targets) < (bound if bound is not None else 2048):
            if bound is None and va != table and (va in self.insn or va in self.owner or va in self.blocks):
                break
            t = self.dword(va)
            if not self.in_text(t):
                break
            targets.append(t)
            self.add_reloc(va, t, 'jump-table')
            for k in range(4):
                self.table_bytes.add(va + k)
            va += 4
        self.jump_tables.append({'jump': ins.address, 'table': table, 'entries': len(targets),
                                 'bound': bound, 'section': self.section_of(table)})
        return targets

    def table_bound(self, jump_addr, index_reg):
        """Entry count from the guarding 'cmp idx, N; ja/jae default' a few instructions back."""
        prev = [a for a in range(jump_addr - 1, jump_addr - 40, -1) if a in self.insn]
        cmp_seen = None
        for a in prev[:8]:
            ins = self.decode(a)
            if ins.mnemonic in ('ja', 'jae', 'jbe', 'jb'):
                cmp_seen = ins.mnemonic
                continue
            if cmp_seen and ins.mnemonic == 'cmp' and len(ins.operands) == 2 \
                    and ins.operands[0].type == X.X86_OP_REG and ins.operands[1].type == X.X86_OP_IMM:
                reg = ins.operands[0].reg
                if reg == index_reg or ins.reg_name(reg)[-2:] == ins.reg_name(index_reg)[-2:]:
                    n = ins.operands[1].imm & 0xFFFFFFFF
                    return n + 1 if cmp_seen == 'ja' else n
        return None

    def note_getproc(self, call_addr):
        for a in range(call_addr - 1, call_addr - 48, -1):
            if a not in self.insn:
                continue
            ins = self.decode(a)
            if ins.mnemonic == 'push' and ins.operands[0].type == X.X86_OP_IMM:
                v = ins.operands[0].imm & 0xFFFFFFFF
                if self.in_image(v) and not self.in_text(v):
                    s = self.cstring(v)
                    if s:
                        self.getproc.append({'call': call_addr, 'name': s})
                        return

    def cstring(self, va, limit=128):
        o = va - self.base
        raw = self.img[o:o + limit].split(b'\0', 1)[0]
        if raw and all(32 <= c < 127 for c in raw):
            return raw.decode()
        return None

    def run(self):
        self.pending = []
        self.deferred = []
        entry = self.base + self.pe.OPTIONAL_HEADER.AddressOfEntryPoint
        self.functions[entry] = 'entry'
        self.trace(entry, 'entry')
        if hasattr(self.pe, 'DIRECTORY_ENTRY_TLS'):
            cb = self.pe.DIRECTORY_ENTRY_TLS.struct.AddressOfCallBacks
            while cb and self.in_image(cb):
                t = self.dword(cb)
                if not t:
                    break
                self.tls_callbacks.append(t)
                self.functions[t] = 'tls-callback'
                self.trace(t, 'tls-callback')
                cb += 4
        self.drain_pending()
        self.scan_data()
        self.pending.extend((t, f'data-ptr@{fixup:#x}') for fixup, t in sorted(self.data_code_ptrs.items()))
        self.drain_pending()
        while self.deferred:
            batch, self.deferred = self.deferred, []
            for t, src in batch:
                if t not in self.insn and t not in self.owner and t not in self.table_bytes:
                    self.try_entry(t, src)
                    self.drain_pending()
        self.sweep_gaps()
        self.classify_data_ptrs()

    def drain_pending(self):
        # Pointers that look like function starts are traced first; the rest wait until
        # all call-reached code is decoded so they cannot claim bytes out of alignment.
        while self.pending:
            batch, self.pending = self.pending, []
            for t, src in batch:
                if t in self.insn or t in self.owner or t in self.table_bytes:
                    continue
                if self.looks_like_start(t):
                    self.try_entry(t, src)
                else:
                    self.deferred.append((t, src))

    def looks_like_start(self, t):
        o = t - self.base
        prev = self.img[o - 1]
        return prev in (0xCC, 0x90, 0xC3) or self.img[o - 3] == 0xC2 or (t % 16 == 0 and prev == 0x00)

    def scan_data(self):
        self.data_ptr_counts = collections.Counter()
        self.unaligned_candidates = 0
        self.text_like_rejected = 0

        def texty(va):
            """4 bytes that read as text: 1-3 ASCII chars + NUL padding, or two UTF-16 chars,
            or 4 ASCII chars."""
            if not self.base <= va < self.end - 3:
                return False
            b = self.img[va - self.base:va - self.base + 4]
            pr = lambda c: 0x20 <= c < 0x7F
            if b[1] == 0 and b[3] == 0 and pr(b[0]) and pr(b[2]):
                return True                      # UTF-16LE pair
            n = len(b.split(b'\0', 1)[0])
            return n >= 2 and all(pr(c) for c in b[:n]) and all(c == 0 for c in b[n:])

        for s in self.sections:
            if s['name'] in ('.text', '.rsrc'):
                continue
            lo, hi = s['va'], s['va'] + min(s['vsize'], s['raw'])
            for va in range(lo, hi - 3):
                v = self.dword(va)
                if not self.in_image(v):
                    continue
                if va % 4:
                    self.unaligned_candidates += 1
                    continue
                if texty(va) and (texty(va - 4) or texty(va + 4)):
                    # Part of a run of text (ASCII or UTF-16) whose bytes happen to fall in
                    # the image's address range; a string, not a pointer.
                    self.uncertain[va] = (v, 'data-text-like')
                    self.text_like_rejected += 1
                    continue
                if self.in_text(v):
                    self.data_code_ptrs[va] = v
                elif self.section_of(v) == '.rsrc':
                    # SRW does not emit the resource section; under the original-address
                    # model (G2e) it is mapped in place, so these stay literal.
                    self.uncertain[va] = (v, 'data-ptr:rsrc-literal')
                else:
                    target_section = self.section_of(v) or 'header (uncertain)'
                    self.add_reloc(va, v, f'data-ptr:{target_section}')
                    self.data_ptr_counts[f'{s["name"]}->{target_section}'] += 1

    def classify_data_ptrs(self):
        for va, t in self.data_code_ptrs.items():
            if t in self.insn:
                self.add_reloc(va, t, 'data-code-ptr')
                self.data_ptr_counts['data->code (instruction start)'] += 1
            else:
                self.add_reloc(va, t, 'data-code-ptr-unresolved', certain=False)
                self.data_ptr_counts['data->code (not an instruction start)'] += 1

    def sweep_gaps(self):
        total = 0
        for _ in range(10):
            found = 0
            for lo, hi in self.gaps():
                a = self.skip_fill(lo, hi)
                if a >= hi:
                    continue
                if a in self.data_in_text:
                    continue
                chunk = self.img[a - self.base:a - self.base + 6]
                source = 'prologue-heuristic' if any(chunk.startswith(p) for p in PROLOGUES) else 'gap-probe'
                if self.try_entry(a, source):
                    self.drain_pending()
                    found += 1
            total += found
            if not found:
                break
        self.heuristic_functions = total

    def skip_fill(self, lo, hi):
        a = lo
        while a < hi:
            o = a - self.base
            if self.img[o] == 0xCC:
                a += 1
                continue
            f = next((f for f in FILLERS if self.img[o:o + len(f)] == f and a + len(f) <= hi), None)
            if f is None:
                break
            a += len(f)
        return a

    def gaps(self):
        covered = sorted([(a, a + s) for a, (s, _) in self.insn.items()] +
                         [(t, t + 1) for t in self.table_bytes if self.in_text(t)])
        out, cur = [], self.text_lo
        for lo, hi in covered:
            if lo > cur:
                out.append((cur, lo))
            cur = max(cur, hi)
        if cur < self.text_hi:
            out.append((cur, self.text_hi))
        return out

    def report(self):
        text_size = self.text_hi - self.text_lo
        code_bytes = sum(s for s, _ in self.insn.values())
        padding, unclassified = 0, []
        targets = sorted({t for t, _ in self.relocs.values()} | {t for t, _ in self.uncertain.values()})
        for lo, hi in self.gaps():
            body = self.img[lo - self.base:hi - self.base]
            pad = sum(1 for c in body if c in PADDING)
            if pad == len(body) or self.skip_fill(lo, hi) >= hi:
                padding += len(body)
                continue
            i = bisect.bisect_left(targets, lo)
            referenced = i < len(targets) and targets[i] < hi
            unclassified.append({'start': lo, 'end': hi, 'size': hi - lo, 'non_padding': len(body) - pad,
                                 'class': 'data-in-text (referenced)' if referenced else 'unreferenced',
                                 'index_table_user': next((t['user'] for t in self.index_tables
                                                           if lo <= t['address'] < hi), None)})
        dll_strings = sorted({m.decode().lower() for m in re.findall(rb'[A-Za-z0-9_]+\.dll', bytes(self.img))})
        indirect_counts = collections.Counter(
            (i['type'], 'import' if i['kind'].startswith('import') else i['kind']) for i in self.indirect)
        imported = collections.Counter(i['kind'] for i in self.indirect if i['kind'].startswith('import'))
        return {
            'text_size': text_size, 'code_bytes': code_bytes, 'jump_table_bytes': len(self.table_bytes),
            'padding_bytes': padding, 'unclassified_bytes': sum(u['size'] for u in unclassified),
            'unclassified_regions': unclassified,
            'instructions': len(self.insn), 'functions': len(self.functions),
            'function_sources': dict(collections.Counter(v.split('@')[0] for v in self.functions.values())),
            'heuristic_functions': self.heuristic_functions, 'tls_callbacks': self.tls_callbacks,
            'jump_tables': self.jump_tables, 'index_tables': self.index_tables,
            'indirect_counts': {f'{k[0]}:{k[1]}': v for k, v in indirect_counts.items()},
            'import_call_sites': dict(imported), 'indirect_sites': self.indirect,
            'bad_decodes': self.bad, 'overlaps': self.overlaps,
            'relocations_certain': len(self.relocs),
            'relocations_by_kind': dict(collections.Counter(k for _, k in self.relocs.values())),
            'relocations_uncertain': len(self.uncertain),
            'uncertain_by_kind': dict(collections.Counter(k for _, k in self.uncertain.values())),
            'uncertain_sites': [{'fixup': f, 'target': t, 'kind': k} for f, (t, k) in sorted(self.uncertain.items())],
            'unaligned_data_candidates': self.unaligned_candidates,
            'data_pointer_counts': dict(self.data_ptr_counts),
            'instruction_families': dict(self.stats), 'top_mnemonics': self.mnemonics.most_common(40),
            'dll_name_strings': dll_strings, 'getprocaddress_names': self.getproc,
        }


def write_markdown(path, r, meta):
    def pct(x):
        return f'{100.0 * x / r["text_size"]:.2f}%'
    L = ['# Execution model — Custom Edition 1.10 BThaloce.exeBT\n',
         f'Generated by BTscripts/audit-executable.pyBT on {meta["date"]} for the accepted profile '
         f'BT{PROFILE_ID}BT (SHA-256 BT{meta["sha256"]}BT). Addresses and counts only; no game bytes. '
         f'Full private manifest: BT{meta["evidence"]}BT. **G2a status: {meta["verdict"]}.**\n',
         '## Image\n', '| Section | VA | Virtual size | Raw size | Flags |\n|---|---|---|---|---|']
    for s in meta['sections']:
        L.append(f"| BT{s['name']}BT | BT{s['va']:#x}BT | BT{s['vsize']:#x}BT | BT{s['raw']:#x}BT | BT{s['characteristics']:#010x}BT |")
    L.append(f"\nImage base BT{meta['base']:#x}BT, entry point BT{meta['entry']:#x}BT, relocations **stripped** "
             f"(BTIMAGE_FILE_RELOCS_STRIPPEDBT, directory size 0). TLS directory present; callbacks: "
             f"{', '.join(hex(t) for t in r['tls_callbacks']) or 'none'}.\n")
    L += ['## Imports\n', '| Module | Kind | Functions |\n|---|---|---|']
    for m in meta['imports']:
        L.append(f"| BT{m['dll']}BT | {'delay' if m['delay'] else 'static'} | {m['count']} |")
    L.append('\nDLL names present as strings (candidates for BTLoadLibraryBT): '
             + ', '.join(f'BT{d}BT' for d in r['dll_name_strings']) + '.')
    gp = sorted({g['name'] for g in r['getprocaddress_names']})
    L.append(f"\nBTGetProcAddressBT names recovered from call sites ({len(gp)}): "
             + (', '.join(f'BT{g}BT' for g in gp) if gp else 'none recovered') + '.\n')
    L += ['## Code coverage of BT.textBT\n', '| Class | Bytes | Share |\n|---|---|---|']
    for k, label in (('code_bytes', 'Decoded instructions'), ('jump_table_bytes', 'Jump tables'),
                     ('padding_bytes', 'Padding gaps (int3/nop only)'), ('unclassified_bytes', 'Unclassified')):
        L.append(f'| {label} | {r[k]:,} | {pct(r[k])} |')
    L.append(f"\n{r['instructions']:,} instructions from {r['functions']:,} function entries "
             f"(sources: {', '.join(f'{k} {v}' for k, v in sorted(r['function_sources'].items()))}). "
             f"{len(r['jump_tables'])} jump tables; {len(r['index_tables'])} byte index-table references from BTmovzxBT. "
             f"{len(r['bad_decodes'])} traced addresses failed to decode or left BT.textBT; "
             f"{len(r['overlaps'])} traced addresses overlapped existing code.\n")
    big = sorted(r['unclassified_regions'], key=lambda u: -u['size'])
    by_class = collections.Counter(u['class'] for u in big)
    size_by_class = collections.Counter()
    for u in big:
        size_by_class[u['class']] += u['size']
    L += ['### Unclassified regions\n',
          f"{len(big)} regions: " + ', '.join(f"{c} {by_class[c]} regions / {size_by_class[c]:,} bytes" for c in sorted(by_class))
          + '. "Referenced" means a recovered address points into the region, so it is data or a table used by code; '
          '"unreferenced" regions have no recovered reference and are listed individually in the private manifest. '
          'Largest 25:\n',
          '| Start | Size | Non-padding bytes | Class | Index-table user |\n|---|---|---|---|---|']
    for u in big[:25]:
        L.append(f"| BT{u['start']:#x}BT | {u['size']} | {u['non_padding']} | {u['class']} | "
                 f"{('BT' + hex(u['index_table_user']) + 'BT') if u['index_table_user'] else ''} |")
    L += ['\n## Indirect control flow\n', '| Site kind | Count |\n|---|---|']
    for k, v in sorted(r['indirect_counts'].items(), key=lambda kv: -kv[1]):
        L.append(f'| BT{k}BT | {v} |')
    L.append('\nEvery BTregisterBT, BTvtable-or-structBT, BTabsolute-pointerBT and BTindexed-memoryBT site needs a finite '
             'guest-target set in the dispatch table (G2e). Import sites map to the narrow Windows runtime.\n')
    L += ['## Recovered relocations\n',
          f"BTgenerated/analysis/{PROFILE_ID}/relocations.csvBT has **{r['relocations_certain']:,}** entries (SRW format BTfixup,targetBT).\n",
          '| Kind | Count |\n|---|---|']
    for k, v in sorted(r['relocations_by_kind'].items(), key=lambda kv: -kv[1]):
        L.append(f'| BT{k}BT | {v:,} |')
    L.append(f"\nExcluded as uncertain: **{r['relocations_uncertain']:,}** "
             f"({', '.join(f'{k} {v}' for k, v in sorted(r['uncertain_by_kind'].items()))}). "
             f"Unaligned in-range dwords in data (not treated as pointers): {r['unaligned_data_candidates']:,}.\n")
    L.append('Data pointers by section: ' + ', '.join(f'{k} {v:,}' for k, v in sorted(r['data_pointer_counts'].items())) + '.\n')
    L.append('Relocation accuracy matters differently per address model. If guest memory keeps the original 32-bit '
             'addresses (the G2e direction), data-to-data pointers need no rewriting; what must be exact is every '
             'value used as a **code** address, because each must map to a compiled function. Uncertain arithmetic '
             'immediates and data dwords that point into BT.textBT but not at a decoded instruction are validated with '
             'oracle runs when a translated path reaches them.\n')
    L += ['## Instruction families\n', '| Family | Count |\n|---|---|']
    for k, v in sorted(r['instruction_families'].items(), key=lambda kv: -kv[1]):
        L.append(f'| BT{k}BT | {v:,} |')
    L.append('\nTop mnemonics: ' + ', '.join(f'BT{m}BT {n:,}' for m, n in r['top_mnemonics'][:25]) + '.\n')
    path.write_text('\n'.join(L).replace('BT', chr(96)) + '\n')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--executable', type=pathlib.Path)
    a = ap.parse_args()
    profile = json.loads((ROOT / 'config/profiles' / f'{PROFILE_ID}.json').read_text())
    exe = a.executable or ROOT / profile['original_root'] / profile['executable']
    digest = sha256(exe)
    if digest != profile['accepted_sha256']:
        sys.exit('FAIL: executable does not match the accepted profile hash')
    audit = Audit(exe)
    audit.run()
    demoted = audit.demote_mid_instruction_targets()
    problems = audit.verify_relocs()
    if problems:
        sys.exit('FAIL: relocation self-check: ' + '; '.join(problems[:10]))
    r = audit.report()
    r['demoted_mid_instruction'] = demoted
    today = datetime.date.today().isoformat()
    stamp = datetime.datetime.utcnow().strftime('%Y%m%dT%H%M%SZ')
    evid = ROOT / 'docs' / 'artifacts' / today / 'G2a' / f'audit-{stamp}'
    evid.mkdir(parents=True, exist_ok=True)
    out = ROOT / 'generated' / 'analysis' / PROFILE_ID
    out.mkdir(parents=True, exist_ok=True)
    with open(out / 'relocations.csv', 'w') as f:
        for fixup in sorted(audit.relocs):
            target, kind = audit.relocs[fixup]
            f.write(f"{fixup:#x},{target:#x},{'i' if kind.endswith(':imagebase') else ''}\n")
    # SRW hints derived from the audit: relocation targets inside .text are code
    # entries only when the audit decoded an instruction there; all others are data.
    text_targets = {t for t, _ in audit.relocs.values() if audit.in_text(t)}
    code_targets = sorted(t for t in text_targets if t in audit.insn)
    data_targets = sorted(t for t in text_targets if t not in audit.insn)
    srw = out / 'srw'
    srw.mkdir(exist_ok=True)
    (srw / 'fixup_interpret_as_code.sci').write_text(''.join(f'loc_{t:X}\n' for t in code_targets))
    (srw / 'fixup_do_not_interpret_as_code.sci').write_text(''.join(f'loc_{t:X}\n' for t in data_targets))
    r['srw_hints'] = {'fixup_interpret_as_code': len(code_targets), 'fixup_do_not_interpret_as_code': len(data_targets)}
    # Every decoded instruction start, for tools that re-decode with another decoder.
    with open(out / 'instructions.u32', 'wb') as f:
        for addr in sorted(audit.insn):
            f.write(struct.pack('<I', addr))
    with open(out / 'functions.json', 'w') as f:
        json.dump({f'{a:#x}': src for a, src in sorted(audit.functions.items())}, f)
    (evid / 'audit.json').write_text(json.dumps(r, indent=1, default=str) + '\n')
    (evid / 'relocations.sha256').write_text(sha256(out / 'relocations.csv') + '  relocations.csv\n')
    mods = collections.Counter((i['dll'], i['delay']) for i in audit.imports.values())
    meta = {'date': today, 'sha256': digest, 'evidence': str(evid.relative_to(ROOT)),
            'sections': audit.sections, 'base': audit.base,
            'entry': audit.base + audit.pe.OPTIONAL_HEADER.AddressOfEntryPoint,
            'imports': [{'dll': d, 'delay': dl, 'count': c}
                        for (d, dl), c in sorted(mods.items(), key=lambda x: (x[0][1], x[0][0]))],
            'verdict': 'PASS' if r['unclassified_bytes'] < 0.02 * r['text_size'] else 'INCOMPLETE (unclassified code remains)'}
    write_markdown(ROOT / 'docs' / 'EXECUTION-MODEL.md', r, meta)
    print(f"{meta['verdict']}: {r['instructions']:,} instructions, {r['functions']:,} functions, "
          f"code {r['code_bytes']:,}/{r['text_size']:,} bytes, tables {r['jump_table_bytes']:,}, padding {r['padding_bytes']:,}, "
          f"unclassified {r['unclassified_bytes']:,} in {len(r['unclassified_regions'])} regions, "
          f"relocations {r['relocations_certain']:,} (+{r['relocations_uncertain']:,} uncertain), "
          f"indirect sites {len(r['indirect_sites']):,}, jump tables {len(r['jump_tables'])}, "
          f"bad {len(r['bad_decodes'])}, overlaps {len(r['overlaps'])}")
    print('evidence', evid.relative_to(ROOT))
    return 0


if __name__ == '__main__':
    sys.exit(main())
