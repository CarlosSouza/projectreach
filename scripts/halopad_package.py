"""Versioned prepared-data format. No extraction or execution of package contents here."""
import hashlib
import json
import os
from pathlib import Path
import re
import stat
import tempfile
import unicodedata
import zipfile

SCHEMA = 1
MAX_FILES = 4096
MAX_FILE = 2 * 1024**3
MAX_TOTAL = 8 * 1024**3
MAX_MANIFEST = 2 * 1024**2
STOCK_TOP = {'haloce.exe', 'strings.dll', 'eula.dll', 'eula.rtf', 'keystone.dll',
             'ksimeui.dll', 'binkw32.dll', 'msvcr71.dll', 'ogg.dll', 'vorbis.dll',
             'vorbisfile.dll', 'config.txt', 'ksml.xsd', 'xiph_license.txt'}
STOCK_DIRS = {'maps', 'content', 'controls', 'shaders'}


class PackageError(ValueError):
    pass


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':'), ensure_ascii=True).encode('utf-8')


def digest(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def safe_name(name):
    if not isinstance(name, str) or not name or len(name.encode('utf-8')) > 512:
        raise PackageError('Invalid package path')
    parts = name.split('/')
    if any(p in ('', '.', '..') or p[-1:] in (' ', '.') for p in parts):
        raise PackageError(f'Unsafe package path: {name!r}')
    if any(ord(c) < 32 or ord(c) == 127 or c in '\\:' for c in name):
        raise PackageError(f'Unsafe package path: {name!r}')
    return unicodedata.normalize('NFC', name).casefold()


def unique_names(names):
    seen = set()
    prefixes = {}
    for name in names:
        key = safe_name(name)
        parts = name.split('/')
        for count in range(1, len(parts) + 1):
            prefix = '/'.join(parts[:count])
            folded = safe_name(prefix)
            if folded in prefixes and prefixes[folded] != prefix:
                raise PackageError(f'Ambiguous path component: {prefix}')
            prefixes[folded] = prefix
        if key in seen:
            raise PackageError(f'Duplicate or ambiguous path: {name}')
        seen.add(key)
    for key in seen:
        if any('/'.join(key.split('/')[:i]) in seen for i in range(1, len(key.split('/')))):
            raise PackageError(f'File/directory collision: {key}')


def inventory(root):
    root = Path(root)
    if root.is_symlink() or not root.is_dir():
        raise PackageError('Source must be a regular directory')
    found = {}
    for directory, dirs, files in os.walk(root, followlinks=False):
        for name in dirs + files:
            path = Path(directory) / name
            mode = path.lstat().st_mode
            if not (stat.S_ISREG(mode) or stat.S_ISDIR(mode)):
                raise PackageError(f'Links and special files are unsupported: {path.relative_to(root)}')
        for name in files:
            path = Path(directory) / name
            found[path.relative_to(root).as_posix()] = path
    unique_names(found)
    return found


def record(path):
    size = Path(path).stat().st_size
    if size > MAX_FILE:
        raise PackageError('File exceeds package size limit')
    return {'size': size, 'sha256': digest(path)}


def check_records(records):
    if not isinstance(records, dict) or not records or len(records) > MAX_FILES:
        raise PackageError('Invalid file inventory')
    unique_names(records)
    total = 0
    for row in records.values():
        if not isinstance(row, dict) or set(row) != {'size', 'sha256'}:
            raise PackageError('Invalid file record')
        if type(row['size']) is not int or not 0 <= row['size'] <= MAX_FILE:
            raise PackageError('Invalid file size')
        if not isinstance(row['sha256'], str) or not re.fullmatch('[0-9a-f]{64}', row['sha256']):
            raise PackageError('Invalid SHA-256')
        total += row['size']
    if total > MAX_TOTAL:
        raise PackageError('Package exceeds total size limit')


def load_json(data):
    def pairs(rows):
        result = {}
        for key, value in rows:
            if key in result:
                raise PackageError(f'Duplicate JSON key: {key}')
            result[key] = value
        return result
    try:
        return json.loads(data, object_pairs_hook=pairs)
    except (ValueError, UnicodeError) as exc:
        raise PackageError(f'Invalid JSON: {exc}') from exc


def create_identity(profile, target, native_inputs, data_root, stock_manifest):
    if stock_manifest['profile'] != profile['id']:
        raise PackageError('Stock manifest belongs to a different profile')
    stock = {}
    for row in stock_manifest['files']:
        name = row['path']
        safe_name(name)
        if name.casefold() in STOCK_TOP or name.split('/')[0].casefold() in STOCK_DIRS:
            if name in stock:
                raise PackageError('Duplicate stock manifest entry')
            stock[name] = {key: row[key] for key in ('size', 'sha256')}
    check_records(stock)
    folded = {name.casefold(): value for name, value in stock.items()}
    if folded.get(profile['executable'].casefold(), {}).get('sha256') != profile['accepted_sha256']:
        raise PackageError('Stock executable differs from the locked profile')
    for module in profile['modules'].values():
        if 'root' not in module and folded.get(module['file'].casefold(), {}).get('sha256') != module['sha256']:
            raise PackageError('Stock module differs from the compiled profile')
    files = inventory(data_root)
    files.pop('core-identity.json', None)
    inert = {name: record(path) for name, path in files.items()}
    check_records(inert)
    code = {name: record(path) for name, path in native_inputs.items()}
    check_records(code)
    identity = dict(schema=SCHEMA, profile=profile['id'], target=target,
                    executable_sha256=profile['accepted_sha256'], native_inputs=code,
                    core_data=inert, stock_files=stock)
    identity['id'] = hashlib.sha256(canonical(identity)).hexdigest()
    return identity


def validate_identity(identity):
    fields = {'schema', 'id', 'profile', 'target', 'executable_sha256', 'native_inputs', 'core_data', 'stock_files'}
    if not isinstance(identity, dict) or set(identity) != fields or type(identity['schema']) is not int or identity['schema'] != SCHEMA:
        raise PackageError('Unsupported core identity schema')
    for field in ('profile', 'target'):
        if not isinstance(identity[field], str) or not identity[field]:
            raise PackageError('Invalid core identity')
    unsigned = {k: v for k, v in identity.items() if k != 'id'}
    if identity['id'] != hashlib.sha256(canonical(unsigned)).hexdigest():
        raise PackageError('Core identity checksum mismatch')
    for field in ('native_inputs', 'core_data', 'stock_files'):
        check_records(identity[field])
    return identity


def read_identity(app_data):
    path = Path(app_data) / 'core-identity.json'
    if path.stat().st_size > MAX_MANIFEST:
        raise PackageError('Core identity too large')
    return validate_identity(load_json(path.read_bytes()))


def expected_files(identity):
    return {**{'game/' + k: v for k, v in identity['stock_files'].items()},
            **{'core-data/' + k: v for k, v in identity['core_data'].items()}}


def manifest_for(identity):
    return {'schema': SCHEMA, 'format': 'halopad-data', 'profile': identity['profile'],
            'core_id': identity['id'], 'files': expected_files(identity)}


def sources_for(root, expected):
    found = inventory(root)
    folded = {safe_name(name): path for name, path in found.items()}
    selected = {}
    for name, wanted in expected.items():
        path = folded.get(safe_name(name))
        if path is None or record(path) != wanted:
            raise PackageError(f'Missing or changed required file: {name}')
        selected[name] = path
    return selected


def prepare(game, app_data, output):
    identity = read_identity(app_data)
    files = {'game/' + k: v for k, v in sources_for(game, identity['stock_files']).items()}
    files.update({'core-data/' + k: v for k, v in sources_for(app_data, identity['core_data']).items()})
    manifest = manifest_for(identity)
    check_records(manifest['files'])
    output = Path(output)
    if not output.name.endswith('.halopad.zip'):
        raise PackageError('Output must end in .halopad.zip')
    if output.exists() or output.is_symlink():
        raise PackageError('Output already exists; choose a new filename')
    for root in (Path(game).resolve(), Path(app_data).resolve()):
        if output.resolve() == root or root in output.resolve().parents:
            raise PackageError('Output must be outside the input folders')
    output.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary = tempfile.mkstemp(prefix='.halopad-package-', dir=output.parent)
    os.close(fd)
    try:
        with zipfile.ZipFile(temporary, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=6, allowZip64=True) as archive:
            payloads = [('manifest.json', canonical(manifest))] + sorted(files.items())
            for name, value in payloads:
                info = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
                info.compress_type = zipfile.ZIP_DEFLATED
                info.create_system = 3
                info.external_attr = (stat.S_IFREG | 0o600) << 16
                with archive.open(info, 'w') as dest:
                    if isinstance(value, bytes):
                        dest.write(value)
                    else:
                        with value.open('rb') as source:
                            for block in iter(lambda: source.read(1024 * 1024), b''):
                                dest.write(block)
        # Verify the produced bytes, detecting source changes during preparation too.
        verify(temporary, identity)
        # Atomic, no-clobber publication: both names are on the output volume.
        os.link(temporary, output)
    finally:
        Path(temporary).unlink(missing_ok=True)
    return manifest


def verify(path, identity):
    validate_identity(identity)
    expected = manifest_for(identity)
    with zipfile.ZipFile(path) as archive:
        entries = archive.infolist()
        if len(entries) != len(expected['files']) + 1 or len(entries) > MAX_FILES:
            raise PackageError('Unexpected number of archive entries')
        unique_names(row.filename for row in entries)
        names = {row.filename: row for row in entries}
        if set(names) != set(expected['files']) | {'manifest.json'}:
            raise PackageError('Archive has missing or unexpected entries')
        for row in entries:
            mode = row.external_attr >> 16
            if row.is_dir() or stat.S_IFMT(mode) not in (0, stat.S_IFREG):
                raise PackageError('Archive links and special files are forbidden')
            if row.flag_bits & 1 or row.compress_type not in (zipfile.ZIP_STORED, zipfile.ZIP_DEFLATED):
                raise PackageError('Unsupported encrypted or compressed archive')
            limit = MAX_MANIFEST if row.filename == 'manifest.json' else expected['files'][row.filename]['size']
            if row.file_size > limit or row.file_size > MAX_FILE:
                raise PackageError('Archive file exceeds declared limit')
        raw = archive.read('manifest.json')
        if canonical(load_json(raw)) != canonical(expected):
            raise PackageError('Manifest does not match the expected compiled core and stock files')
        for name, wanted in expected['files'].items():
            if names[name].file_size != wanted['size']:
                raise PackageError(f'Size mismatch: {name}')
            h = hashlib.sha256()
            size = 0
            with archive.open(name) as stream:
                for block in iter(lambda: stream.read(1024 * 1024), b''):
                    size += len(block)
                    if size > wanted['size']:
                        raise PackageError('Expanded file exceeds limit')
                    h.update(block)
            if size != wanted['size'] or h.hexdigest() != wanted['sha256']:
                raise PackageError(f'Content mismatch: {name}')
    return expected
