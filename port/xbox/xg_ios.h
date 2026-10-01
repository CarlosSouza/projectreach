/* xg_ios.h: the Xbox engine's entry points for the iOS app (xg_ios.m). */
#ifndef XG_IOS_H
#define XG_IOS_H
#import <UIKit/UIKit.h>
#include "xg_touch_input.h"

/* the view the game draws into; make it on the main thread before starting */
UIView *xg_ios_make_view(CGRect frame);
/* call from the main thread when the view's size changes */
void xg_ios_view_resized(void);
/* loads the game image and starts the game on its own thread; 0 on success */
int xg_ios_start(const char *image_path, const char *data_root, const char *save_root);

/* player 1's touch gamepad (xg_touch.m): axes in SDL order (left x, left y,
 * right x, right y, left trigger, right trigger; -1..1, y down), buttons as
 * bits numbered by SDL_GamepadButton */
void xg_ios_set_touch_pad(const struct xg_touch_pad *state);
void xg_ios_clear_touch_pad(void);
/* nonzero while a game controller is player 1's */
int xg_ios_controller_connected(void);

/* the Xbox-layout touch gamepad; add it above the game view */
@interface XGTouchPad : UIView
@end

#endif
