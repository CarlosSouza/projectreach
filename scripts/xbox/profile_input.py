"""Guarded, read-only input-context bridge for the private paired guest/host."""
import hashlib
import pathlib

INPUT = pathlib.Path('source/input/input_xbox.c')
IMPORTS = pathlib.Path('port/android/host_imports.list')
HASHES = {
    INPUT: 'c923599871b14b6f2ba05e3ee5c5412e61147ae09c063d4d975259ba91309c8b',
    IMPORTS: 'd0e34312ca324e6fda257f9844eba6b460938fff787c90e0c3d5ec210c81e4f0',
}
ANCHOR = b'void input_frame_begin(\n\tvoid)\n{\n'
HEADERS = b'#include "interface/player_ui.h"\n'
INSERT = b'''#ifdef HALO_ANDROID
    /* HaloPad input context v1: scalar values, no guest memory offsets. */
    {
        extern void host_halopad_input_context_v1(unsigned int, unsigned int, unsigned int, unsigned int);
        struct game_input_preferences preferences;
        unsigned int low = 0, high = 0;
        unsigned int i;
        input_abstraction_get_local_player_preferences(0, &preferences);
        for (i = 0; i < 12; i++) {
            unsigned int value = preferences.game_control_to_xbox_buttons[i];
            /* Preserve invalidity rather than truncate an unknown enum. */
            if (value > 15) { low = high = 0; break; }
            if (i < 8) low |= value << (i * 4);
            else high |= value << ((i - 8) * 4);
        }
        host_halopad_input_context_v1(ui_widgets_active() ? 1u : 0u,
            low, high, (unsigned int)preferences.joystick_controls);
    }
#endif
'''
IMPORT = b'\nhost_halopad_input_context_v1\n'


def recipe():
    return str(sorted((str(p), h) for p, h in HASHES.items())).encode() + ANCHOR + HEADERS + INSERT + IMPORT


def adapt(path, original):
    if hashlib.sha256(original).hexdigest() != HASHES[path]:
        raise ValueError('Input bridge source changed; review upstream first')
    if path == IMPORTS:
        return original + IMPORT
    if original.count(ANCHOR) != 1 or original.count(HEADERS) != 1:
        raise ValueError('Input bridge anchors changed; review upstream first')
    return original.replace(HEADERS, HEADERS + b'#include "interface/ui_widget.h"\n').replace(ANCHOR, ANCHOR + INSERT)


def changes(engine):
    result = []
    for path in HASHES:
        original = (engine / path).read_bytes()
        result.append((engine / path, original, adapt(path, original)))
    return result
