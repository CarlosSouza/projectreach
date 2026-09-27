; Hand-maintained SRW replacements for haloce.exe 1.0.10.0621 (engineering profile).
; Format: loc_<addr>,<bytes>,<llasm lines separated by |>. See docs/G2C-SRW-CAPABILITY.md.
;
; 0x5cce49 CRT local-unwind helper: cmp esi, fs:[0] / je 0x5cce5b / push esi / call 0x5cd698 / add esp, 4
loc_5CCE49,18,;cmp esi, [fs:0]|call x86_read_fs_dword 0x0|cmoveq esi, tmp0, tmp1, 1, 0|;je loc_5CCE5B|ctcallnz tmp1, hp_loc_5CCE5B|;push esi|PUSH esi|;call loc_5CD698|PUSH hp_loc_5CCE58|tcall loc_5CD698|endp|proc hp_loc_5CCE58|;add esp, 4|add esp, esp, 4|tcall hp_loc_5CCE5B|endp|proc hp_loc_5CCE5B ; SEH frame compare
