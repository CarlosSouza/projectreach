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
