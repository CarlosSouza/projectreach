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

import hpmodule
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
        'enter', 'pushal', 'popal', 'pushfd', 'sahf', 'lahf', 'xlatb', 'icebp', 'int1', 'fwait', 'int'}


def sha256(path):
    return hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()


class Audit:
    def __init__(self, exe, pe_relocs=None):
        # pe_relocs: the file's own base-relocation table (DLLs). It is ground truth for
        # which dwords are addresses, so data pointers come from it instead of the scan.
        self.pe_relocs = pe_relocs
        # With a relocation table, an address the table lists is inside an instruction's
        # operand or in data, never at an instruction's first byte (opcodes precede operands).
        self.pe_fixups = {f for f, _ in pe_relocs} if pe_relocs is not None else set()
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
        self.negative_candidates = []   # (jump, base, index register) for tables with no entries
        self.negative_index_jumps = []
        self.displaced_tables = {}      # table base -> byte distance to its first real entry
        self.noret = {}                 # call target -> never returns (see is_noreturn)

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
            if self.in_text(target) and target in self.owner and target not in self.displaced_tables:
                del self.relocs[fixup]
                self.uncertain[fixup] = (target, kind + ':mid-instruction')
                demoted += 1
        return demoted

    def x87_idiom(self, a, ins):
        """'wait' before an x87 instruction, and 'sahf' after 'fnstsw ax' (with or without a
        'wait' between): the x87 status idiom compiled code uses to branch on a comparison.
        The CRT's fmod (0x5cdc14, reached only from its _trandisp table at 0x61fac0) loops
        on 'fprem; wait; fnstsw ax; wait; sahf; jp'."""
        # MSVC also emits 'wait' before a __try state change ('mov dword ptr [ebp-4], n',
        # C7 45 FC imm32) so pending x87 exceptions are raised in the old scope; the
        # __except bodies at 0x546a7e and 0x546c28 continue into such sequences.
        o = a - self.base
        if ins.mnemonic in ('wait', 'fwait'):
            if self.img[o - 2:o] == b'\xdf\xe0':                  # after 'fnstsw ax'
                return True
            if self.img[o + 1:o + 4] == b'\xc7\x45\xfc':
                return True
            nxt = self.decode(a + ins.size)
            return nxt is not None and nxt.mnemonic.startswith('f')
        if ins.mnemonic == 'sahf':
            return self.img[o - 2:o] == b'\xdf\xe0' or (self.img[o - 1] == 0x9b and self.img[o - 3:o - 1] == b'\xdf\xe0')
        return False

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
                if a in self.pe_fixups:
                    return 'starts at a relocation fixup (data)'
                ins = self.decode(a)
                if ins is None:
                    return 'undecodable'
                if a == start and ins.mnemonic == 'int3':
                    return 'starts with int3 (padding or data)'   # no compiled function begins with a breakpoint
                if any((a + k) in self.insn or (a + k) in self.owner or (a + k) in self.table_bytes
                       for k in range(1, ins.size)):
                    return 'overlaps known code'
                o = a - self.base
                if (ins.mnemonic in JUNK or self.img[o:o + 2] == b'\0\0') and not self.x87_idiom(a, ins):
                    # The CRT's CPUID-availability check uses 'pushfd; pop r32'
                    # (0x5cf251, reached from the initializer table): that idiom is code.
                    nxt = self.decode(a + ins.size) if ins.mnemonic == 'pushfd' else None
                    if not (nxt is not None and nxt.mnemonic == 'pop' and nxt.operands
                            and nxt.operands[0].type == X.X86_OP_REG and nxt.operands[0].size == 4):
                        return 'data-like instruction'
                seen[a] = ins.size
                if len(seen) > limit:
                    return 'too long'
                m = ins.mnemonic
                if m == 'ret' and ins.operands and ins.operands[0].type == X.X86_OP_IMM:
                    n = ins.operands[0].imm & 0xFFFF
                    if n % 4 or n > 0x100:                  # compiled code pops a few whole arguments
                        return 'data-like instruction'
                if m in ENDS or m.startswith('ret'):
                    break
                op = ins.operands[0] if ins.operands else None
                if m in UNCOND:
                    if op is not None and op.type == X.X86_OP_IMM:
                        work.append(op.imm & 0xFFFFFFFF)
                    break
                if capstone.CS_GRP_JUMP in ins.groups and op is not None and op.type == X.X86_OP_IMM:
                    work.append(op.imm & 0xFFFFFFFF)
                if m == 'call' and op is not None and op.type == X.X86_OP_IMM and self.is_noreturn(op.imm & 0xFFFFFFFF):
                    break
                a += ins.size
        starts = sorted(seen)
        for x, y in zip(starts, starts[1:]):
            if x + seen[x] > y:
                return 'self-overlapping'
        return None

    def is_string_at(self, t):
        """A NUL-terminated run of printable characters (ASCII at least 6, UTF-16LE at least 3): a string
        that an immediate or data pointer names (msxml4.dll keeps its strings in .text)."""
        o = t - self.base
        b = self.img[o:o + 256]
        pr = lambda c: 0x20 <= c < 0x7F or c in (9, 10, 13)
        n = 0
        while n < len(b) and pr(b[n]):
            n += 1
        # 6: 'push imm32' of an address in a low image ('h8@b' + NUL) is code with 4 printable bytes
        if n >= 6 and n < len(b) and b[n] == 0:
            return True
        k = 0
        while 2 * k + 1 < len(b) and pr(b[2 * k]) and b[2 * k + 1] == 0:
            k += 1
        return k >= 3 and 2 * k + 1 < len(b) and b[2 * k] == 0 and b[2 * k + 1] == 0

    def try_entry(self, target, source):
        if target in self.insn:
            return True
        if not source.startswith(('call@', 'entry', 'export:', 'tls-callback')) and self.is_string_at(target):
            self.probe_rejects['string'] += 1
            self.data_in_text.setdefault(target, source)
            return False
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
                        if self.is_noreturn(t):
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

    def is_noreturn(self, t):
        """A call to t never returns: an import thunk to a no-return import, or straight-line
        code (no branch or return before it) that exits the process/thread or throws a C++
        exception (MSVC _CxxThrowException: RaiseException of a copied template whose code is
        0xE06D7363 and whose flags are EXCEPTION_NONCONTINUABLE)."""
        if t in self.noret:
            return self.noret[t]
        self.noret[t] = False                     # recursion guard
        result = self.is_noreturn_thunk(t) or self._straight_noreturn(t)
        self.noret[t] = result
        return result

    def _straight_noreturn(self, t):
        if not self.in_text(t):
            return False
        a, template = t, None
        for _ in range(64):
            ins = self.decode(a)
            if ins is None:
                return False
            m, ops = ins.mnemonic, ins.operands
            if m == 'mov' and len(ops) == 2 and ops[0].type == X.X86_OP_REG and ops[1].type == X.X86_OP_IMM:
                v = ops[1].imm & 0xFFFFFFFF
                if self.in_image(v) and not self.in_text(v):
                    template = v
            if m == 'call':
                op = ops[0]
                if op.type == X.X86_OP_MEM and op.mem.base == 0 and op.mem.index == 0:
                    name = self.imports.get(op.mem.disp & 0xFFFFFFFF, {}).get('name')
                    if name in ('ExitProcess', 'ExitThread', 'FatalAppExitA', 'FatalAppExitW'):
                        return True
                    if name == 'RaiseException':
                        return template is not None and self.dword(template) == 0xE06D7363 and self.dword(template + 4) == 1
                elif op.type == X.X86_OP_IMM and self.is_noreturn(op.imm & 0xFFFFFFFF):
                    return True
            if m in ENDS or m.startswith('ret') or capstone.CS_GRP_JUMP in ins.groups:
                return False
            a += ins.size
        return False

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
        bound, bound_kind = self.table_bound(ins.address, op.mem.index)
        if bound and self.index_negated(ins.address, op.mem.index):
            # 'neg idx; jmp [base + idx*4]' (MSVC memmove): entries run downward from base.
            targets = []
            for k in range(bound):
                va = table - 4 * k
                t = self.dword(va)
                if not self.in_text(t):
                    break
                targets.append(t)
                self.add_reloc(va, t, 'jump-table')
                for b in range(4):
                    self.table_bytes.add(va + b)
            self.jump_tables.append({'jump': ins.address, 'table': table - 4 * (len(targets) - 1) if targets else table,
                                     'entries': len(targets), 'holes': 0, 'bound': bound, 'negated': True,
                                     'section': self.section_of(table)})
            return targets
        targets = []
        va = table
        holes = 0
        while len(targets) < (bound if bound is not None else 2048):
            if bound is None and va != table and (va in self.insn or va in self.owner or va in self.blocks):
                break
            t = self.dword(va)
            if not self.in_text(t) and bound is None and t == 0 and targets and holes < 2 \
                    and self.in_text(self.dword(va + 4)) and (va + 4) not in self.insn and (va + 4) not in self.owner:
                # A NULL slot inside an unbounded table (a case that cannot occur) with more
                # entries after it: 0x4a7810 (index from a loop count) has 0 as its fourth of six
                # entries, and the cases after it (0x4a77dd, 0x4a77fa) are reached.
                holes += 1
                va += 4
                continue
            if not self.in_text(t):
                if bound_kind == 'mask' and len(targets) + holes < bound:
                    # e.g. 'and eax, 3' with index 0 never used: an unused slot, not the end
                    # (and possibly the tail of a neighbouring instruction), so it is not
                    # marked as table data.
                    holes += 1
                    va += 4
                    if len(targets) + holes >= bound:
                        break
                    continue
                break
            targets.append(t)
            self.add_reloc(va, t, 'jump-table')
            for k in range(4):
                self.table_bytes.add(va + k)
            va += 4
            if bound_kind == 'mask' and len(targets) + holes >= bound:
                break
        if not targets:
            self.negative_candidates.append((ins.address, table, ins.reg_name(op.mem.index)))
        elif bound_kind == 'mask' and self.dword(table) and not self.in_text(self.dword(table)):
            # Leading unused slot(s): the table base may lie inside a neighbouring
            # instruction. Reference it through the first real entry (SRW displaced label).
            lead = 0
            while not self.in_text(self.dword(table + 4 * lead)):
                lead += 1
            self.displaced_tables[table] = 4 * lead
        self.jump_tables.append({'jump': ins.address, 'table': table, 'entries': len(targets),
                                 'holes': holes, 'bound': bound, 'section': self.section_of(table)})
        return targets

    def index_negated(self, jump_addr, index_reg):
        """True if 'neg index_reg' is among the three instructions before the jump."""
        prev = [a for a in range(jump_addr - 1, jump_addr - 16, -1) if a in self.insn][:3]
        for a in prev:
            ins = self.decode(a)
            if ins.mnemonic == 'neg' and ins.operands and ins.operands[0].type == X.X86_OP_REG \
                    and ins.operands[0].reg == index_reg:
                return True
        return False

    def table_bound(self, jump_addr, index_reg):
        """Entry count from the guarding 'cmp idx, N; ja/jae default' a few instructions back,
        or from 'and idx, mask' (mask 1/3/7/15). Returns (count, 'cmp'|'mask') or (None, None)."""
        prev = [a for a in range(jump_addr - 1, jump_addr - 40, -1) if a in self.insn]
        cmp_seen = None
        for a in prev[:8]:
            ins = self.decode(a)
            if ins.mnemonic == 'and' and len(ins.operands) == 2 and ins.operands[0].type == X.X86_OP_REG \
                    and ins.operands[1].type == X.X86_OP_IMM and ins.operands[0].reg == index_reg \
                    and (ins.operands[1].imm & 0xFFFFFFFF) in (1, 3, 7, 15):
                return (ins.operands[1].imm & 0xFFFFFFFF) + 1, 'mask'
            if ins.mnemonic in ('ja', 'jae', 'jbe', 'jb'):
                cmp_seen = ins.mnemonic
                continue
            if cmp_seen and ins.mnemonic == 'cmp' and len(ins.operands) == 2 \
                    and ins.operands[0].type == X.X86_OP_REG and ins.operands[1].type == X.X86_OP_IMM:
                reg = ins.operands[0].reg
                if reg == index_reg or ins.reg_name(reg)[-2:] == ins.reg_name(index_reg)[-2:]:
                    n = ins.operands[1].imm & 0xFFFFFFFF
                    return (n + 1 if cmp_seen == 'ja' else n), 'cmp'
        return None, None

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
        if hasattr(self.pe, 'DIRECTORY_ENTRY_EXPORT'):
            for e in self.pe.DIRECTORY_ENTRY_EXPORT.symbols:
                t = self.base + e.address
                if self.in_text(t):
                    name = e.name.decode() if e.name else f'#{e.ordinal}'
                    self.functions.setdefault(t, f'export:{name}')
                    self.trace(t, f'export:{name}')
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
        self.follow_text_data_ptrs()
        self.sweep_gaps()
        self.follow_text_data_ptrs()
        self.classify_data_ptrs()
        self.resolve_negative_index_jumps()

    def resolve_negative_index_jumps(self):
        """MSVC's hand-written memcpy indexes a table backwards from a label that is also
        code ('jmp [label + ecx*4]' with ecx in -4..-1). Record the real table and the
        byte distance so the translator can index it from its true start."""
        tables = {j['table']: j for j in self.jump_tables if j['entries'] > 0}
        for jump, base, reg in self.negative_candidates:
            if base not in self.insn:
                continue
            real = [t for t, j in tables.items() if t < base <= t + 4 * (j['entries'] + j.get('holes', 0))]
            if len(real) == 1:
                t = real[0]
                self.negative_index_jumps.append({'jump': jump, 'base': base, 'table': t,
                                                  'delta_entries': (base - t) // 4, 'index': reg,
                                                  'length': self.insn[jump][0]})

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

        if self.pe_relocs is not None:
            self.scan_pe_relocs()
            return
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
                code_run = self.in_text(v) and (self.in_text(self.dword(va - 4)) or self.in_text(self.dword(va + 4)))
                if texty(va) and (texty(va - 4) or texty(va + 4)) and not code_run:
                    # (A run of pointers into .text is a table of code pointers even when its
                    # bytes read as text: addresses 0x5x2xxx-0x5x7exx are printable, such as
                    # the callback table holding 0x566d20 at 0x636b18. classify_data_ptrs keeps
                    # only targets that decode as instruction starts.)
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

    def scan_pe_relocs(self):
        """Data pointers from the file's relocation table. Fixups inside .text belong to
        instructions and jump tables, which tracing finds and checks against this table."""
        for fixup, v in self.pe_relocs:
            if self.in_text(fixup):
                continue
            s = self.section_of(fixup)
            if self.in_text(v):
                self.data_code_ptrs[fixup] = v
            elif self.section_of(v) == '.rsrc':
                self.uncertain[fixup] = (v, 'data-ptr:rsrc-literal')
            else:
                self.add_reloc(fixup, v, f'data-ptr:{self.section_of(v) or "header"}')
                self.data_ptr_counts[f'{s}->{self.section_of(v)}'] += 1

    def follow_text_data_ptrs(self):
        """Constant data inside .text (vtables and tables of a DLL built without .rdata, such as
        msxml4.dll): a listed fixup that no decoded instruction covers is a data pointer; code
        it points at is traced, which can uncover more such tables. Repeats until stable."""
        if self.pe_relocs is None:
            return
        while True:
            found = 0
            for fixup, v in self.pe_relocs:
                if not self.in_text(fixup) or fixup in self.insn or fixup in self.owner or fixup in self.table_bytes:
                    continue
                if fixup in self.data_code_ptrs or fixup in self.relocs or fixup in self.uncertain:
                    continue
                if self.in_text(v):
                    self.data_code_ptrs[fixup] = v
                    if v not in self.insn and v not in self.owner and v not in self.table_bytes:
                        self.pending.append((v, f'text-data-ptr@{fixup:#x}'))
                        found += 1
                elif self.section_of(v) == '.rsrc':
                    self.uncertain[fixup] = (v, 'data-ptr:rsrc-literal')
                else:
                    self.add_reloc(fixup, v, f'data-ptr:{self.section_of(v) or "header"}')
            self.drain_pending()
            while self.deferred:
                batch, self.deferred = self.deferred, []
                for t, src in batch:
                    if t not in self.insn and t not in self.owner and t not in self.table_bytes:
                        self.try_entry(t, src)
                        self.drain_pending()
            if not found:
                return

    def check_against_pe_relocs(self):
        """Every address the audit found in code must be in the file's table; report the
        table's .text fixups the audit did not explain."""
        table = {f for f, _ in self.pe_relocs}
        extra = sorted(f for f in self.relocs if f not in table)
        text_fixups = {f for f in table if self.in_text(f)}
        explained = {f for f in text_fixups if f in self.relocs or f in self.uncertain or f in self.owner or f in self.table_bytes}
        return {'audit_not_in_table': [hex(f) for f in extra[:50]], 'audit_not_in_table_count': len(extra),
                'text_fixups': len(text_fixups), 'text_fixups_unexplained': len(text_fixups - explained),
                'unexplained_sample': [hex(f) for f in sorted(text_fixups - explained)[:50]]}

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
            'negative_index_jumps': self.negative_index_jumps,
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
    hpmodule.add_argument(ap)
    a = ap.parse_args()
    mod = hpmodule.Module(a.module)
    exe = a.executable or mod.exe
    mod.verify_source()
    digest = sha256(mod.source)
    pe_relocs = None if mod.relocs_stripped else hpmodule.pe_relocations(exe)
    audit = Audit(exe, pe_relocs)
    audit.run()
    demoted = audit.demote_mid_instruction_targets() if pe_relocs is None else 0
    if pe_relocs is not None:
        # The file's table decides: drop code-operand guesses it does not list.
        table = dict(pe_relocs)
        for fixup in [f for f in audit.relocs if f not in table]:
            audit.uncertain[fixup] = audit.relocs.pop(fixup)
    problems = audit.verify_relocs()
    if problems:
        sys.exit('FAIL: relocation self-check: ' + '; '.join(problems[:10]))
    r = audit.report()
    r['demoted_mid_instruction'] = demoted
    today = datetime.date.today().isoformat()
    stamp = datetime.datetime.utcnow().strftime('%Y%m%dT%H%M%SZ')
    evid = ROOT / 'docs' / 'artifacts' / today / 'G2a' / (f'audit-{stamp}' if mod.primary else f'audit-{mod.name}-{stamp}')
    evid.mkdir(parents=True, exist_ok=True)
    out = mod.analysis
    out.mkdir(parents=True, exist_ok=True)
    if pe_relocs is None:
        with open(out / 'relocations.csv', 'w') as f:
            for fixup in sorted(audit.relocs):
                target, kind = audit.relocs[fixup]
                f.write(f"{fixup:#x},{target:#x},{'i' if kind.endswith(':imagebase') else ''}\n")
    else:
        r['pe_relocation_check'] = audit.check_against_pe_relocs()
    (out / 'image.bin').write_bytes(audit.pe.get_memory_mapped_image())
    # SRW hints derived from the audit: relocation targets inside .text are code
    # entries only when the audit decoded an instruction there; all others are data.
    # (SRW applies a DLL's own table, so every target in it needs a decision.)
    all_targets = [t for _, t in pe_relocs] if pe_relocs is not None else [t for t, _ in audit.relocs.values()]
    text_targets = {t for t in all_targets if audit.in_text(t)}
    code_targets = sorted(t for t in text_targets if t in audit.insn)
    data_targets = sorted(t for t in text_targets if t not in audit.insn)
    srw = out / 'srw'
    srw.mkdir(exist_ok=True)
    (srw / 'fixup_interpret_as_code.sci').write_text(''.join(f'loc_{t:X}\n' for t in code_targets))
    (srw / 'fixup_do_not_interpret_as_code.sci').write_text(''.join(f'loc_{t:X}\n' for t in data_targets))
    if pe_relocs is not None:
        # A listed address that points inside a decoded instruction (msxml4.dll computes an array
        # base as 'table - k' with lea) is a real address that SRW cannot label. The image is
        # never relocated here, so SRW keeps these as literals (port/patches: literal_fixups.csv).
        literal = sorted(f for f, t in pe_relocs if audit.in_text(t) and t in audit.owner)
        (srw / 'literal_fixups.csv').write_text(''.join(f'{f:#x}\n' for f in literal))
        r['literal_fixups'] = [hex(f) for f in literal]
    noret = sorted(t for t, v in audit.noret.items() if v and t in audit.insn)
    (srw / 'noret_procedures.sci').write_text(''.join(f'loc_{t:X}\n' for t in noret))
    r['noret_procedures'] = [hex(t) for t in noret]
    (srw / 'displaced_labels.sci').write_text(''.join(f'loc_{t:X},{d}\n' for t, d in sorted(audit.displaced_tables.items())))
    r['srw_hints'] = {'fixup_interpret_as_code': len(code_targets), 'fixup_do_not_interpret_as_code': len(data_targets)}
    # Every decoded instruction start, for tools that re-decode with another decoder.
    with open(out / 'instructions.u32', 'wb') as f:
        for addr in sorted(audit.insn):
            f.write(struct.pack('<I', addr))
    with open(out / 'functions.json', 'w') as f:
        json.dump({f'{a:#x}': src for a, src in sorted(audit.functions.items())}, f)
    (evid / 'audit.json').write_text(json.dumps(r, indent=1, default=str) + '\n')
    (out / 'audit.json').write_text(json.dumps(r, indent=1, default=str) + '\n')
    if pe_relocs is None:
        (evid / 'relocations.sha256').write_text(sha256(out / 'relocations.csv') + '  relocations.csv\n')
    mods = collections.Counter((i['dll'], i['delay']) for i in audit.imports.values())
    meta = {'date': today, 'sha256': digest, 'evidence': str(evid.relative_to(ROOT)),
            'sections': audit.sections, 'base': audit.base,
            'entry': audit.base + audit.pe.OPTIONAL_HEADER.AddressOfEntryPoint,
            'imports': [{'dll': d, 'delay': dl, 'count': c}
                        for (d, dl), c in sorted(mods.items(), key=lambda x: (x[0][1], x[0][0]))],
            'verdict': 'PASS' if r['unclassified_bytes'] < 0.02 * r['text_size'] else 'INCOMPLETE (unclassified code remains)'}
    if mod.primary:
        write_markdown(ROOT / 'docs' / 'EXECUTION-MODEL.md', r, meta)
    else:
        print('relocation-table check:', {k: v for k, v in r['pe_relocation_check'].items() if not k.endswith('sample') and not k.endswith('table')})
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
