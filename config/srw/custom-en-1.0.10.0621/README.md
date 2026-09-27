# Hand-maintained SRW inputs — Custom Edition 1.10 (engineering profile)

`scripts/run-srw.py` appends every `.sci`/`.cfg`/`.csv` file in this directory after the generated inputs in `generated/analysis/custom-en-1.0.10.0621/srw/`.

**SRW's `.sci` parser has no comment syntax.** A line containing a comma is parsed as an entry and silently reuses the previous address if its own does not parse. Keep notes here, never in the `.sci` files.

## instruction_replacements.sci

Format: `loc_<hex addr>,<byte length>,<llasm lines separated by |>`. Text after ` ; ` on the last llasm line is an llasm comment.

| Address | Bytes | Replaces | Why |
|---|---|---|---|
| `0x5cce49` | 18 | `cmp esi, fs:[0]` / `je 0x5cce5b` / `push esi` / `call 0x5cd698` / `add esp, 4` | CRT local-unwind helper. The `cmp` with an FS operand has no llasm backend case. The `je` target is only referenced from inside the replaced bytes, so the replacement defines its own continuation procedures. |

## global_aliases.sci

Format: `loc_<hex addr>,<name>`. Exports a translated procedure as the C-callable `c_<name>(_cpu *)` for slice harnesses.

| Address | Name | Used by |
|---|---|---|
| `0x59f2a2` | `halo_crc32` | `scripts/run-slice-crc32.py` (G2d slice 1: stdcall CRC32) |
| `0x5c83f0` | `halo_memmove` | `scripts/run-slices.py` (CRT memmove/memcpy: rep movsd + jump tables) |
| `0x5c88c0` | `halo_strrchr` | `scripts/run-slices.py` (CRT strrchr: repne scasb with std/cld) |
| `0x5c88f0` | `halo_strncmp` | `scripts/run-slices.py` (CRT strncmp: jecxz, rep cmpsb) |
| `0x5834d7` | `halo_vec3_transform_coord` | `scripts/run-slices.py` (x87 math, generic D3DX-style transform) |
| `0x583b65` | `halo_vec4_transform` | `scripts/run-slices.py` (x87 math) |
| `0x4434a0` | `halo_map_header_valid` | `scripts/run-slices.py` (reads a map file's 0x800-byte header through CreateFileA/ReadFile/CloseHandle and validates it; eax = name, esi = buffer) |
| `0x5ccac7` | `halo_entry` | G3: the PE entry point (MSVC CRT startup, which calls WinMain `0x5445e0`). SRW's llasm mode does not root the entry point, and a global alias is SRW's way to add a translation root. Without it, the CRT startup and WinMain are not translated. |

## Translated DLLs

`keystone/` and `ksimeui/` hold the same kinds of files for `Keystone.dll` and `ksimeui.dll` (`scripts/srw-pipeline.sh <build> --module <name>`).

| File | Entry | Why |
|---|---|---|
| `keystone/global_aliases.sci` | `loc_102DB232,keystone_entry` | DLL entry point (`_DllMainCRTStartup`); SRW's llasm mode does not root it |
| `ksimeui/global_aliases.sci` | `loc_1000D2EC,ksimeui_entry` | same |
| `keystone/instruction_replacements.sci` | `0x102dbabd`, 18 bytes | the same CRT local-unwind helper as the executable's `0x5cce49` (`cmp esi, fs:[0]`) |
| `keystone/instruction_replacements.sci` | `0x1026bc4d`, 5 bytes | `and eax, offset` (zlib `inflateInit2`: the check function or 0). SRW's backend has no `and` with an address operand; the replacement puts the address in a temporary. The next instruction does not read flags. |
| `keystone/instruction_replacements.sci` | `0x10263ac7`, 3 bytes | `imul byte [ecx+0x1d]` (libpng's pixel depth = bit depth × channels): `ax = al × m8`, signed. The next instructions do not read flags. |
| `keystone/instruction_replacements.sci` | `0x102e143d`, `0x102e208d` (4 bytes) | `bts [esp], eax` in the CRT's `strspn`/`strpbrk` family: sets bit `eax` of the 256-bit table on the stack, and CF gets the old bit. |
| `keystone/instruction_replacements.sci` | `0x102e1458`, `0x102e20a1` (4 bytes) | `bt [esp], eax`: CF = bit `eax` of that table (read by the following `jae`). |
| `controls/instruction_replacements.sci` | `0x1033ffc1`, `0x10351603`, `0x1035344b`, `0x1035fa43` (5 bytes) | `and eax, offset` after `neg`/`sbb`/`not`: selects 0 or a string address ("true", "false", "vstarget"). Each is followed by `pop esi` and `ret`. |
