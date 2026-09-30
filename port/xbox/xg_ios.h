/* xg_ios.h: the Xbox engine's entry points for the iOS app (xg_ios.m). */
#ifndef XG_IOS_H
#define XG_IOS_H
#import <UIKit/UIKit.h>

/* the view the game draws into; make it on the main thread before starting */
UIView *xg_ios_make_view(CGRect frame);
/* call from the main thread when the view's size changes */
void xg_ios_view_resized(void);
/* loads the game image and starts the game on its own thread; 0 on success */
int xg_ios_start(const char *image_path, const char *data_root, const char *save_root);

#endif
