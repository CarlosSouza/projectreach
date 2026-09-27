#!/usr/bin/env python3
"""Read-only PE inventory. Unknown hashes and wrong versions fail closed.
Reports are private and are never accepted input locks automatically.
"""
import argparse
import datetime
import hashlib
import json
import pathlib
import sys
import uuid
import pefile

ROOT = pathlib.Path(__file__).resolve().parents[1]

def sha256(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()

def decoded(value):
    return value.decode('utf-8', errors='replace') if isinstance(value, bytes) else value

def inventory(path):
    if path.is_symlink() or not path.is_file():
        raise ValueError('input must be a regular, non-symlink file')
    if path.stat().st_size > 256 * 1024 * 1024:
        raise ValueError('PE exceeds the inspection size limit')
    digest = sha256(path)
    pe = pefile.PE(str(path), fast_load=False)
    versions = []
    for value in getattr(pe, 'VS_FIXEDFILEINFO', []):
        versions.append('.'.join(str(v) for v in [value.FileVersionMS >> 16,
            value.FileVersionMS & 65535, value.FileVersionLS >> 16, value.FileVersionLS & 65535]))
    def imports(attribute):
        return [{'dll': decoded(m.dll), 'symbols': [
            {'name': decoded(s.name), 'ordinal': s.ordinal, 'iat_va': s.address}
            for s in m.imports]} for m in getattr(pe, attribute, [])]
    resources = []
    def walk_resources(directory, prefix):
        for entry in directory.entries:
            key = str(entry.name) if entry.name is not None else entry.id
            if hasattr(entry, 'directory'):
                walk_resources(entry.directory, prefix + [key])
            elif hasattr(entry, 'data'):
                resources.append({'path': prefix + [key], 'rva': entry.data.struct.OffsetToData,
                                  'size': entry.data.struct.Size})
    if hasattr(pe, 'DIRECTORY_ENTRY_RESOURCE'):
        walk_resources(pe.DIRECTORY_ENTRY_RESOURCE, [])
    report = {'sha256': digest, 'size': path.stat().st_size, 'versions': versions,
        'machine': pe.FILE_HEADER.Machine, 'optional_magic': pe.OPTIONAL_HEADER.Magic,
        'image_base': pe.OPTIONAL_HEADER.ImageBase,
        'entry_point_rva': pe.OPTIONAL_HEADER.AddressOfEntryPoint,
        'size_of_image': pe.OPTIONAL_HEADER.SizeOfImage,
        'sections': [{'name': decoded(s.Name.rstrip(b'\0')), 'rva': s.VirtualAddress,
            'virtual_size': s.Misc_VirtualSize, 'file_offset': s.PointerToRawData,
            'file_size': s.SizeOfRawData, 'characteristics': s.Characteristics}
            for s in pe.sections], 'imports': imports('DIRECTORY_ENTRY_IMPORT'),
        'delay_imports': imports('DIRECTORY_ENTRY_DELAY_IMPORT'),
        'directories': [{'name': d.name, 'rva_or_file_offset': d.VirtualAddress, 'size': d.Size}
            for d in pe.OPTIONAL_HEADER.DATA_DIRECTORY],
        'resources': resources,
        'relocation_blocks': [{'rva': b.struct.VirtualAddress,
            'entries': [{'type': e.type, 'rva': e.rva} for e in b.entries]}
            for b in getattr(pe, 'DIRECTORY_ENTRY_BASERELOC', [])],
        'tls': None, 'warnings': pe.get_warnings(),
        'coverage': 'NOT_ANALYZED; directory inventory is not code-path coverage',
        'provenance': 'OPERATOR_SUPPLIED; accepted original-PC-key route and baseline not established'}
    if hasattr(pe, 'DIRECTORY_ENTRY_TLS'):
        t = pe.DIRECTORY_ENTRY_TLS.struct
        report['tls'] = {key: getattr(t, key) for key in [
            'StartAddressOfRawData', 'EndAddressOfRawData', 'AddressOfIndex',
            'AddressOfCallBacks', 'SizeOfZeroFill', 'Characteristics']}
    pe.close()
    if sha256(path) != digest:
        raise ValueError('input changed during inspection')
    return report

def profile_errors(report, profile):
    errors = []
    if report['machine'] != 0x14c or report['optional_magic'] != 0x10b:
        errors.append('input is not PE32/x86')
    def numeric(version):
        return tuple(int(part) for part in version.split('.'))

    if [numeric(v) for v in report['versions']] != [numeric(profile['version'])]:
        errors.append('wrong or ambiguous executable version')
    if not profile.get('accepted_sha256'):
        errors.append('profile hash has not been accepted')
    elif report['sha256'] != profile['accepted_sha256']:
        errors.append('exact accepted executable hash mismatch')
    return errors

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile', required=True, choices=['custom-en-1.0.10.0621', 'retail-en-1.0.10.0621'])
    parser.add_argument('--executable', type=pathlib.Path, required=True)
    args = parser.parse_args()
    profile = json.loads((ROOT / 'config/profiles' / (args.profile + '.json')).read_text())
    try:
        report = inventory(args.executable)
    except (ValueError, OSError, pefile.PEFormatError) as exc:
        print('FAIL: cannot inspect PE:', exc, file=sys.stderr)
        return 2
    errors = profile_errors(report, profile)
    report.update({'profile': args.profile, 'identity_result': 'FAIL' if errors else 'PASS',
                   'errors': errors, 'baseline_result': 'NOT_RUN'})
    out = ROOT / 'docs/artifacts' / datetime.date.today().isoformat() / 'G1' / ('inspect-' + uuid.uuid4().hex[:12])
    out.mkdir(parents=True)
    (out / 'pe-inventory.json').write_text(json.dumps(report, indent=2) + '\n')
    print(report['identity_result'], args.profile, 'version=' + ','.join(report['versions']))
    for error in errors:
        print(' -', error)
    print('Private report:', out.relative_to(ROOT) / 'pe-inventory.json')
    return 2 if errors else 0

if __name__ == '__main__':
    sys.exit(main())
