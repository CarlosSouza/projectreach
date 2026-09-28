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
@property(nonatomic, copy) NSArray<NSString *> *recentServers;
@end

@interface HPOverlay : UIView
@property(nonatomic, weak, nullable) id<HPOverlayDelegate> delegate;
/* In a game (Halo's current map is not its menu map): the touch controls show and take touches. */
@property(nonatomic) BOOL inGame;
/* Frames Halo presented in the last second, for the FPS counter; -1 hides it. */
- (void)setFramesPerSecond:(int)fps;
/* Types text into Halo as a player would on a keyboard ("\n" is Enter). */
+ (void)typeText:(NSString *)text;
/* Press and release a Windows key (virtual key, set-1 scan code). */
+ (void)tapKey:(uint32_t)vk scan:(uint32_t)scan;
- (void)refreshControllerVisibility;
- (void)clearTouchInput;
@end

NS_ASSUME_NONNULL_END
