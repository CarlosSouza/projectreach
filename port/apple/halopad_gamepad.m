/* HaloPad Apple host: game controllers (macOS and iOS), from the GameController framework.
 * Every connected controller with an extended gamepad profile is reported; DirectInput
 * (port/runtime/halopad_dinput.c) presents each as a Windows XP Xbox 360 controller. */
#import <GameController/GameController.h>
#include <string.h>
#include "../runtime/halopad_input.h"

int halopad_host_gamepads(hp_gamepad *out, int max)
{
    static NSMapTable *ids;                               /* controller -> id, while it exists */
    static uint32_t next_id = 1;
    int n = 0;
    @autoreleasepool {
        if (!ids) ids = [NSMapTable weakToStrongObjectsMapTable];
        for (GCController *c in GCController.controllers) {
            GCExtendedGamepad *g = c.extendedGamepad;
            if (!g || n == max) continue;
            NSNumber *id_ = [ids objectForKey:c];
            if (!id_) { id_ = @(next_id++); [ids setObject:id_ forKey:c]; }
            hp_gamepad *p = &out[n++];
            memset(p, 0, sizeof *p);
            p->id = id_.unsignedIntValue;
            p->lx = g.leftThumbstick.xAxis.value; p->ly = g.leftThumbstick.yAxis.value;
            p->rx = g.rightThumbstick.xAxis.value; p->ry = g.rightThumbstick.yAxis.value;
            p->lt = g.leftTrigger.value; p->rt = g.rightTrigger.value;
            GCControllerButtonInput *b[10] = {g.buttonA, g.buttonB, g.buttonX, g.buttonY, g.leftShoulder, g.rightShoulder,
                                              g.buttonOptions, g.buttonMenu, g.leftThumbstickButton, g.rightThumbstickButton};
            for (int i = 0; i < 10; i++) if (b[i].pressed) p->buttons |= 1u << i;
            int up = g.dpad.up.pressed, down = g.dpad.down.pressed, left = g.dpad.left.pressed, right = g.dpad.right.pressed;
            static const int dir[3][3] = {{7, 0, 1}, {6, -1, 2}, {5, 4, 3}};   /* [row: up/none/down][col: left/none/right] */
            p->dpad = dir[up ? 0 : down ? 2 : 1][left ? 0 : right ? 2 : 1];
        }
    }
    return n;
}
