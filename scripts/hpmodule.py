"""Translated guest modules of the Custom Edition 1.10 profile.

haloce.exe is the program. Keystone.dll (the chat edit box and chat log Halo draws
through its own Direct3D device) and ksimeui.dll (Keystone's static IME dependency)
ship with the game and run as translated code at their preferred bases, like the
executable. Each module has its accepted SHA-256, an ignored analysis directory for
generated hints and a tracked directory for hand-maintained SRW overrides.

Halo's paths are the original single-module paths; the DLLs live one level down.
"""
import argparse
import glob
import json
import pathlib
import struct

import pefile

ROOT = pathlib.Path(__file__).resolve().parents[1]
PROFILE_ID = 'custom-en-1.0.10.0621'
PROFILE = json.loads((ROOT / 'config' / 'profiles' / f'{PROFILE_ID}.json').read_text())
ANALYSIS = ROOT / 'generated' / 'analysis' / PROFILE_ID
HAND = ROOT / 'config' / 'srw' / PROFILE_ID
INSTALL_DIR = 'C:\\Program Files\\Microsoft Games\\Halo Custom Edition'   # as the guest sees it

MODULES = {
    'haloce': {'file': PROFILE['executable'], 'sha256': PROFILE['accepted_sha256']},
    **{name: dict(m) for name, m in PROFILE.get('modules', {}).items()},
}


class Module:
    def __init__(self, name):
        if name not in MODULES:
            raise SystemExit(f'unknown module {name!r}; known: {", ".join(MODULES)}')
        m = MODULES[name]
        self.name = name
        self.relpath = m['file']                              # path in the install directory
        self.file = pathlib.PurePosixPath(self.relpath).name
        self.sha256 = m['sha256']
        self.source = ROOT / m.get('root', PROFILE['original_root']) / self.relpath
        # where the guest finds it (GetModuleFileNameA): the install directory unless stated
        self.guest_path = m.get('guest_path') or INSTALL_DIR + '\\' + self.relpath.replace('/', '\\')
        self.primary = name == 'haloce'
        self.analysis = ANALYSIS if self.primary else ANALYSIS / 'modules' / name
        self.hand = HAND if self.primary else HAND / name
        self.stem = 'haloce' if self.primary else name       # SRW input/output file stem
        self.rebase = int(m['rebase'], 16) if 'rebase' in m else None
        # The file every tool reads: the accepted file itself, or for a DLL whose preferred
        # base is taken, the same file with its own relocation table applied at 'rebase'
        # (what the Windows loader would map), derived deterministically.
        self.exe = self.analysis / 'rebased' / self.file if self.rebase is not None else self.source
        if self.rebase is not None and not self.exe.exists():
            self.derive()
        pe = pefile.PE(str(self.exe), fast_load=True)
        self.base = pe.OPTIONAL_HEADER.ImageBase
        self.size = pe.OPTIONAL_HEADER.SizeOfImage
        self.entry = self.base + pe.OPTIONAL_HEADER.AddressOfEntryPoint
        self.is_dll = bool(pe.FILE_HEADER.Characteristics & 0x2000)
        text = next(s for s in pe.sections if s.Name.rstrip(b'\0') == b'.text')
        self.text_lo = self.base + text.VirtualAddress
        self.text_hi = self.text_lo + text.Misc_VirtualSize
        self.relocs_stripped = bool(pe.FILE_HEADER.Characteristics & 0x1)

    def verify_source(self):
        import hashlib
        if hashlib.sha256(self.source.read_bytes()).hexdigest() != self.sha256:
            raise SystemExit(f'FAIL: {self.relpath} does not match the accepted profile hash')

    def derive(self):
        """Apply the file's own base relocations for 'rebase': every IMAGE_REL_BASED_HIGHLOW
        dword gains (rebase - ImageBase) and the header records the new base. Nothing else
        in the file changes."""
        self.verify_source()
        data = bytearray(self.source.read_bytes())
        pe = pefile.PE(data=bytes(data))
        if not hasattr(pe, 'DIRECTORY_ENTRY_BASERELOC'):
            raise SystemExit(f'{self.relpath}: no relocation table to rebase with')
        delta = (self.rebase - pe.OPTIONAL_HEADER.ImageBase) & 0xFFFFFFFF
        for block in pe.DIRECTORY_ENTRY_BASERELOC:
            for e in block.entries:
                if e.type == 0:
                    continue
                if e.type != 3:
                    raise SystemExit(f'{self.relpath}: relocation type {e.type} at {e.rva:#x}')
                off = pe.get_offset_from_rva(e.rva)
                v = struct.unpack_from('<I', data, off)[0]
                struct.pack_into('<I', data, off, (v + delta) & 0xFFFFFFFF)
        struct.pack_into('<I', data, pe.OPTIONAL_HEADER.get_file_offset() + 28, self.rebase)   # ImageBase
        self.exe.parent.mkdir(parents=True, exist_ok=True)
        self.exe.write_bytes(bytes(data))

    def image(self):
        return (self.analysis / 'image.bin').read_bytes()

    def audit(self):
        """This module's latest audit (the executable's older audits live only in evidence)."""
        own = self.analysis / 'audit.json'
        if own.exists():
            return json.loads(own.read_text())
        if not self.primary:
            raise SystemExit(f'no audit for {self.name}; run scripts/audit-executable.py --module {self.name}')
        return json.loads(pathlib.Path(sorted(glob.glob(str(ROOT / 'docs/artifacts/*/G2a/audit-*/audit.json')))[-1]).read_text())

    def relocations(self):
        """[(fixup, target)]: the audit's recovered table when the file has none, else the PE's own."""
        if self.relocs_stripped:
            out = []
            for line in (self.analysis / 'relocations.csv').read_text().splitlines():
                f, t = line.split(',')[:2]
                out.append((int(f, 16), int(t, 16)))
            return out
        return pe_relocations(self.exe)


def pe_relocations(path):
    pe = pefile.PE(str(path))
    img = pe.get_memory_mapped_image()
    base = pe.OPTIONAL_HEADER.ImageBase
    out = []
    for block in getattr(pe, 'DIRECTORY_ENTRY_BASERELOC', []):
        for e in block.entries:
            if e.type == 3:                          # IMAGE_REL_BASED_HIGHLOW
                f = e.rva + base
                out.append((f, struct.unpack_from('<I', img, e.rva)[0]))
            elif e.type != 0:                        # ABSOLUTE is padding
                raise SystemExit(f'{path.name}: unsupported relocation type {e.type} at {e.rva:#x}')
    return sorted(out)


def add_argument(ap: argparse.ArgumentParser):
    ap.add_argument('--module', default='haloce', choices=sorted(MODULES),
                    help='guest module (default: the executable)')


def all_modules():
    return [Module(n) for n in MODULES]
