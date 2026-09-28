/* HaloPad touch controls and the three-dot menu (G8/G9), adapted from SunPad's mobile
 * interaction layer (ref/sunpad: apple/ios/SunPadGameOverlay.mm at e43f0ea, GPL-3.0; see
 * docs/SUNPAD-TRANSFER.md). The overlay sits above the game view. In a game it shows Halo's PC
 * controls as touch controls and turns them into the keys and mouse input Halo already reads
 * (halopad_host_post_input); in Halo's menus it passes touches through to the game view, where a
 * tap is a click. The menu holds what Halo's own menus do not: joining a server by address,
 * Halo's console and an on-screen keyboard, display options, touch-control settings and a problem
 * report. */
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@class HPOverlay;

@protocol HPOverlayDelegate <NSObject>
/* The display options changed (aspect ratio). */
- (void)overlayDisplayChanged:(HPOverlay *)overlay;
/* Show the system keyboard for Halo's text entry (chat, console, names). */
- (void)overlayRequestsKeyboard:(HPOverlay *)overlay;
/* Privacy-safe state for a problem report. */
- (NSString *)overlayDiagnostics:(HPOverlay *)overlay;
@end

typedef NS_ENUM(NSInteger, HPAspectMode) { HPAspectOriginal = 0, HPAspectFill = 1 };

@interface HPSettings : NSObject
+ (instancetype)shared;
@property(nonatomic) CGFloat controlOpacity;        /* 0.25..1 */
@property(nonatomic) CGFloat controlSize;           /* 0.70..1.35 */
@property(nonatomic) CGFloat lookSensitivity;       /* 0.25..3 */
@property(nonatomic) BOOL hideWithController;
@property(nonatomic) BOOL hideTouchControls;
@property(nonatomic) BOOL showFPS;
@property(nonatomic) HPAspectMode aspect;
@property(nonatomic) BOOL leftHanded;               /* swap movement and aiming/action sides */
@property(nonatomic) BOOL showCaptions;             /* the small captions under the icons */
@property(nonatomic) NSInteger ringSpacing;         /* 0 compact, 1 normal, 2 spread: gaps between action targets */
@property(nonatomic, copy) NSArray<NSString *> *recentServers;
@end

@interface HPOverlay : UIView
@property(nonatomic, weak, nullable) id<HPOverlayDelegate> delegate;
/* A gameplay map is loaded; Halo may also have its pause/child menu open. */
@property(nonatomic) BOOL inGame;
/* Actual Halo widget state: release/hide gameplay targets and retain an Escape/Back target. */
@property(nonatomic) BOOL haloMenuVisible;
/* Hide and release gameplay controls while the system keyboard occupies the game display. */
@property(nonatomic) BOOL softwareKeyboardVisible;
/* Enabled only after the core has configured the independent touch controller. */
@property(nonatomic) BOOL analogMoveReady;
/* Frames Halo presented in the last second, for the FPS counter; -1 hides it. */
- (void)setFramesPerSecond:(int)fps;
/* Types text into Halo as a player would on a keyboard ("\n" is Enter). */
+ (void)typeText:(NSString *)text;
/* Press and release a Windows key (virtual key, set-1 scan code). */
+ (void)tapKey:(uint32_t)vk scan:(uint32_t)scan;
/* Scene focus, on the main thread: losing focus cancels unfinished typing and releases
   its delivered keys. Regaining focus starts empty; it never replays a partial command. */
+ (void)setTextInputActive:(BOOL)active;
- (void)refreshControllerVisibility;
- (void)clearTouchInput;
/* Development self-test (HALOPAD_TOUCH_SELFTEST): drive the controls through the same handlers
   their touches use. */
- (BOOL)driveControl:(NSString *)identifier down:(BOOL)down;
- (void)driveMoveX:(float)x y:(float)y;
- (void)driveAimX:(float)x y:(float)y;
- (void)driveLookX:(CGFloat)dx y:(CGFloat)dy;
@end

NS_ASSUME_NONNULL_END
