/*
 * HaloPadXbox.m: the launch picker (Halo PC or Halo Xbox) and the Xbox game's
 * screen. Compiled into HaloPad only when the Xbox engine was built on this
 * Mac (scripts/xbox/build-ios.sh; docs/XBOX-ENGINE.md). HaloPadApp.m finds
 * HPEngineChooserMake through a weak reference and, without it, starts the
 * PC game as before.
 *
 * One engine runs per launch: both claim the same guest memory, so choosing
 * the other game means closing HaloPad and opening it again.
 */
#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include "xg_ios.h"
#include "xg_xiso.h"

/* system link: iOS lets apps broadcast only with a restricted entitlement, so
 * the game searches the addresses the player lists instead (upstream's
 * network.broadcast, read when the game starts) */
static NSString *const link_key = @"HaloPadXboxLinkAddresses";

static NSString *own_addresses(void)
{
	NSMutableArray *found = [NSMutableArray array];
	struct ifaddrs *list = NULL, *entry;
	if (getifaddrs(&list) == 0)
	{
		for (entry = list; entry; entry = entry->ifa_next)
		{
			char text[INET_ADDRSTRLEN];
			if (!entry->ifa_addr || entry->ifa_addr->sa_family != AF_INET || (entry->ifa_flags & IFF_LOOPBACK) ||
				!(entry->ifa_flags & IFF_UP) || strncmp(entry->ifa_name, "en", 2))
				continue;
			inet_ntop(AF_INET, &((struct sockaddr_in *)entry->ifa_addr)->sin_addr, text, sizeof(text));
			if (strncmp(text, "169.254.", 8))
				[found addObject:@(text)];
		}
		freeifaddrs(list);
	}
	return found.count ? [found componentsJoinedByString:@", "] : @"not on a network";
}

static NSString *xbox_root(void)
{
	NSString *documents = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject;
	return [documents stringByAppendingPathComponent:@"Halo Xbox"];
}

static NSString *xbox_data(void)
{
	const char *development = getenv("XG_DATA");
	return development ? @(development) : xbox_root();
}

static NSString *xbox_saves(void)
{
	const char *development = getenv("XG_SAVE");
	return development ? @(development) : [xbox_root() stringByAppendingPathComponent:@"save"];
}

static BOOL xbox_has_maps(void)
{
	return [NSFileManager.defaultManager fileExistsAtPath:[xbox_data() stringByAppendingPathComponent:@"maps/ui.map"]];
}

/* ---------- the Xbox game */

@interface HPXboxViewController : UIViewController <UIDocumentPickerDelegate>
@end

@implementation HPXboxViewController
{
	UIView *game;
	XGTouchPad *pad;
	UIButton *link_button;
	UIView *import_panel;
	UILabel *import_status;
	UIProgressView *import_progress;
	UIButton *import_button;
	BOOL started;
}

- (void)loadView
{
	UIView *root = [[UIView alloc] initWithFrame:UIScreen.mainScreen.bounds];
	root.backgroundColor = UIColor.blackColor;
	game = xg_ios_make_view(root.bounds);
	game.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
	[root addSubview:game];
	pad = [[XGTouchPad alloc] initWithFrame:root.bounds];
	pad.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
	pad.hidden = YES;
	[root addSubview:pad];
	link_button = [UIButton buttonWithType:UIButtonTypeSystem];
	[link_button setTitle:@"Link" forState:UIControlStateNormal];
	[link_button setTitleColor:[UIColor colorWithWhite:1 alpha:0.85] forState:UIControlStateNormal];
	link_button.titleLabel.font = [UIFont systemFontOfSize:14 weight:UIFontWeightSemibold];
	link_button.backgroundColor = [UIColor colorWithWhite:0 alpha:0.28];
	link_button.layer.cornerRadius = 14;
	link_button.frame = CGRectMake(20, 16, 60, 32);
	[link_button addTarget:self action:@selector(showLink) forControlEvents:UIControlEventTouchUpInside];
	[root addSubview:link_button];
	self.view = root;
}

- (void)viewSafeAreaInsetsDidChange
{
	[super viewSafeAreaInsetsDidChange];
	link_button.frame = CGRectMake(self.view.safeAreaInsets.left + 20, self.view.safeAreaInsets.top + 16, 60, 32);
}

- (void)showLink
{
	NSString *saved = [NSUserDefaults.standardUserDefaults stringForKey:link_key] ?: @"";
	NSString *message = [NSString stringWithFormat:@"To play system link with other iPads, iPhones or computers running this "
		@"Xbox version, enter their addresses (separated by commas). They enter this device's address: %@.\n\n"
		@"Changes take effect the next time you open Halo Xbox.", own_addresses()];
	UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"System Link" message:message
		preferredStyle:UIAlertControllerStyleAlert];
	[alert addTextFieldWithConfigurationHandler:^(UITextField *field) {
		field.text = saved;
		field.placeholder = @"192.168.1.20, 192.168.1.21";
		field.keyboardType = UIKeyboardTypeNumbersAndPunctuation;
		field.autocorrectionType = UITextAutocorrectionTypeNo;
	}];
	[alert addAction:[UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:nil]];
	[alert addAction:[UIAlertAction actionWithTitle:@"Save" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
		NSString *text = [alert.textFields.firstObject.text stringByReplacingOccurrencesOfString:@" " withString:@""];
		[NSUserDefaults.standardUserDefaults setObject:text forKey:link_key];
	}]];
	[self presentViewController:alert animated:YES completion:nil];
}

- (void)viewDidLayoutSubviews
{
	[super viewDidLayoutSubviews];
	xg_ios_view_resized();
}

- (void)viewDidAppear:(BOOL)animated
{
	[super viewDidAppear:animated];
	if (started)
		return;
	if (xbox_has_maps())
		[self startGame];
	else
	{
		[self showImport];
		/* development: HALOPAD_XBOX_IMPORT=<disc image> imports it as if the player had picked it */
		if (getenv("HALOPAD_XBOX_IMPORT"))
			[self documentPicker:nil didPickDocumentsAtURLs:@[ [NSURL fileURLWithPath:@(getenv("HALOPAD_XBOX_IMPORT"))] ]];
	}
}

- (void)startGame
{
	NSString *image = [NSBundle.mainBundle.bundlePath stringByAppendingPathComponent:@"data/xbox/halo_guest.elf"];
	started = YES;
	import_panel.hidden = YES;
	pad.hidden = NO;
	[NSFileManager.defaultManager createDirectoryAtPath:xbox_saves() withIntermediateDirectories:YES attributes:nil error:nil];
	{
		NSString *addresses = [NSUserDefaults.standardUserDefaults stringForKey:link_key];
		if (addresses.length)
			setenv("HALO_NET_BROADCAST", addresses.UTF8String, 0);
	}
	/* development on a device: XG_FRAME_DUMP_DOCUMENTS=1 saves frames to Documents/xbox-frame.ppm */
	if (getenv("XG_FRAME_DUMP_DOCUMENTS"))
		setenv("XG_FRAME_DUMP", [xbox_root().stringByDeletingLastPathComponent stringByAppendingPathComponent:@"xbox-frame.ppm"].fileSystemRepresentation, 1);
	if (xg_ios_start(image.fileSystemRepresentation, xbox_data().fileSystemRepresentation, xbox_saves().fileSystemRepresentation))
		[self showProblem:@"The Xbox game could not start. Share the diagnostic log from Settings > HaloPad if this keeps happening."];
}

- (void)showProblem:(NSString *)text
{
	UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"Halo Xbox" message:text preferredStyle:UIAlertControllerStyleAlert];
	[alert addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil]];
	[self presentViewController:alert animated:YES completion:nil];
}

- (void)showImport
{
	UIStackView *stack;
	UILabel *title = [UILabel new], *body = [UILabel new];
	import_panel = [UIView new];
	import_panel.backgroundColor = [UIColor colorWithWhite:0.08 alpha:1];
	import_panel.layer.cornerRadius = 20;
	import_panel.translatesAutoresizingMaskIntoConstraints = NO;
	title.text = @"Add your Halo Xbox disc";
	title.font = [UIFont systemFontOfSize:26 weight:UIFontWeightBold];
	title.textColor = UIColor.whiteColor;
	body.text = @"Choose a disc image (.iso or .xiso) of your own Halo: Combat Evolved for Xbox. "
		@"HaloPad copies its maps (about 1.8 GB) into the Halo Xbox folder; you can delete the image afterwards.";
	body.numberOfLines = 0;
	body.textColor = [UIColor colorWithWhite:0.8 alpha:1];
	body.font = [UIFont systemFontOfSize:16];
	import_button = [UIButton buttonWithType:UIButtonTypeSystem];
	[import_button setTitle:@"Choose Disc Image" forState:UIControlStateNormal];
	import_button.titleLabel.font = [UIFont systemFontOfSize:18 weight:UIFontWeightSemibold];
	[import_button addTarget:self action:@selector(pick) forControlEvents:UIControlEventTouchUpInside];
	import_progress = [[UIProgressView alloc] initWithProgressViewStyle:UIProgressViewStyleDefault];
	import_progress.hidden = YES;
	import_status = [UILabel new];
	import_status.textColor = [UIColor colorWithWhite:0.7 alpha:1];
	import_status.numberOfLines = 0;
	import_status.font = [UIFont systemFontOfSize:14];
	stack = [[UIStackView alloc] initWithArrangedSubviews:@[ title, body, import_button, import_progress, import_status ]];
	stack.axis = UILayoutConstraintAxisVertical;
	stack.spacing = 16;
	stack.alignment = UIStackViewAlignmentFill;
	stack.translatesAutoresizingMaskIntoConstraints = NO;
	[import_panel addSubview:stack];
	[self.view addSubview:import_panel];
	[NSLayoutConstraint activateConstraints:@[
		[import_panel.centerXAnchor constraintEqualToAnchor:self.view.centerXAnchor],
		[import_panel.centerYAnchor constraintEqualToAnchor:self.view.centerYAnchor],
		[import_panel.widthAnchor constraintEqualToConstant:520],
		[stack.topAnchor constraintEqualToAnchor:import_panel.topAnchor constant:28],
		[stack.bottomAnchor constraintEqualToAnchor:import_panel.bottomAnchor constant:-28],
		[stack.leadingAnchor constraintEqualToAnchor:import_panel.leadingAnchor constant:28],
		[stack.trailingAnchor constraintEqualToAnchor:import_panel.trailingAnchor constant:-28]]];
}

- (void)pick
{
	UTType *iso = [UTType typeWithFilenameExtension:@"iso"] ?: UTTypeData;
	UTType *xiso = [UTType typeWithFilenameExtension:@"xiso"] ?: UTTypeData;
	UIDocumentPickerViewController *picker = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:@[ iso, xiso, UTTypeData ]];
	picker.delegate = self;
	[self presentViewController:picker animated:YES completion:nil];
}

static void import_progress_update(double fraction, void *context)
{
	static int last = -1;
	int percent = (int)(fraction * 100);
	UIProgressView *progress = (__bridge UIProgressView *)context;
	if (percent == last)
		return;
	last = percent;
	dispatch_async(dispatch_get_main_queue(), ^{ progress.progress = (float)fraction; });
}

- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls
{
	NSURL *url = urls.firstObject;
	NSString *destination = xbox_data();
	UIProgressView *progress = import_progress;
	if (!url)
		return;
	import_button.enabled = NO;
	import_progress.hidden = NO;
	import_status.text = @"Copying the maps from your disc…";
	dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
		char build[64], error[256];
		BOOL access = [url startAccessingSecurityScopedResource];
		int result = xg_extract_maps(url.fileSystemRepresentation, destination.fileSystemRepresentation,
			import_progress_update, (__bridge void *)progress, build, sizeof(build), error, sizeof(error));
		NSString *message = @(error), *maps_build = @(build);
		if (access)
			[url stopAccessingSecurityScopedResource];
		dispatch_async(dispatch_get_main_queue(), ^{
			if (result == 0)
			{
				NSLog(@"HaloPad Xbox: imported maps build %@", maps_build);
				[self startGame];
				return;
			}
			self->import_button.enabled = YES;
			self->import_progress.hidden = YES;
			self->import_status.text = message;
		});
	});
}

- (BOOL)prefersStatusBarHidden { return YES; }
- (BOOL)prefersHomeIndicatorAutoHidden { return YES; }
- (UIInterfaceOrientationMask)supportedInterfaceOrientations { return UIInterfaceOrientationMaskLandscape; }
@end

/* ---------- the launch picker */

@interface HPEngineChooser : UIViewController
@property(nonatomic, copy) UIViewController *(^makePC)(void);
@end

@implementation HPEngineChooser

- (UIButton *)cardWithTitle:(NSString *)title subtitle:(NSString *)subtitle action:(SEL)action
{
	UIButtonConfiguration *configuration = [UIButtonConfiguration filledButtonConfiguration];
	UIButton *button;
	configuration.title = title;
	configuration.subtitle = subtitle;
	configuration.titleAlignment = UIButtonConfigurationTitleAlignmentCenter;
	configuration.baseBackgroundColor = [UIColor colorWithWhite:0.12 alpha:1];
	configuration.baseForegroundColor = UIColor.whiteColor;
	configuration.cornerStyle = UIButtonConfigurationCornerStyleLarge;
	configuration.contentInsets = NSDirectionalEdgeInsetsMake(28, 24, 28, 24);
	configuration.titleTextAttributesTransformer = ^NSDictionary *(NSDictionary *in) {
		NSMutableDictionary *out = [in mutableCopy];
		out[NSFontAttributeName] = [UIFont systemFontOfSize:28 weight:UIFontWeightBold];
		return out;
	};
	configuration.subtitleTextAttributesTransformer = ^NSDictionary *(NSDictionary *in) {
		NSMutableDictionary *out = [in mutableCopy];
		out[NSFontAttributeName] = [UIFont systemFontOfSize:15];
		out[NSForegroundColorAttributeName] = [UIColor colorWithWhite:0.75 alpha:1];
		return out;
	};
	button = [UIButton buttonWithConfiguration:configuration primaryAction:nil];
	[button addTarget:self action:action forControlEvents:UIControlEventTouchUpInside];
	return button;
}

- (void)loadView
{
	UIView *root = [UIView new];
	UILabel *title = [UILabel new], *note = [UILabel new];
	UIStackView *cards, *stack;
	root.backgroundColor = UIColor.blackColor;
	title.text = @"Choose your Halo";
	title.font = [UIFont systemFontOfSize:34 weight:UIFontWeightBold];
	title.textColor = UIColor.whiteColor;
	title.textAlignment = NSTextAlignmentCenter;
	cards = [[UIStackView alloc] initWithArrangedSubviews:@[
		[self cardWithTitle:@"Halo PC" subtitle:@"Custom Edition\nOnline on community servers" action:@selector(choosePC)],
		[self cardWithTitle:@"Halo Xbox" subtitle:@"The Xbox game\nCampaign, split-screen, system link" action:@selector(chooseXbox)] ]];
	cards.axis = UILayoutConstraintAxisHorizontal;
	cards.spacing = 24;
	cards.distribution = UIStackViewDistributionFillEqually;
	note.text = @"The two versions cannot play online together. To switch later, close HaloPad and open it again.";
	note.numberOfLines = 0;
	note.textAlignment = NSTextAlignmentCenter;
	note.textColor = [UIColor colorWithWhite:0.6 alpha:1];
	note.font = [UIFont systemFontOfSize:14];
	stack = [[UIStackView alloc] initWithArrangedSubviews:@[ title, cards, note ]];
	stack.axis = UILayoutConstraintAxisVertical;
	stack.spacing = 28;
	stack.translatesAutoresizingMaskIntoConstraints = NO;
	[root addSubview:stack];
	[NSLayoutConstraint activateConstraints:@[
		[stack.centerXAnchor constraintEqualToAnchor:root.centerXAnchor],
		[stack.centerYAnchor constraintEqualToAnchor:root.centerYAnchor],
		[stack.widthAnchor constraintLessThanOrEqualToConstant:760],
		[stack.leadingAnchor constraintGreaterThanOrEqualToAnchor:root.safeAreaLayoutGuide.leadingAnchor constant:24],
		[stack.trailingAnchor constraintLessThanOrEqualToAnchor:root.safeAreaLayoutGuide.trailingAnchor constant:-24]]];
	self.view = root;
	/* development: HALOPAD_CHOOSE=pc or xbox presses that card once the picker is up */
	if (getenv("HALOPAD_CHOOSE"))
	{
		UIButton *card = cards.arrangedSubviews[strcmp(getenv("HALOPAD_CHOOSE"), "xbox") ? 0 : 1];
		dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 2 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
			[card sendActionsForControlEvents:UIControlEventTouchUpInside];
		});
	}
}

- (void)show:(UIViewController *)controller
{
	UIWindow *window = self.view.window;
	[NSUserDefaults.standardUserDefaults setObject:[controller isKindOfClass:HPXboxViewController.class] ? @"xbox" : @"pc" forKey:@"HaloPadLastEngine"];
	window.rootViewController = controller;
}

- (void)choosePC { [self show:self.makePC()]; }
- (void)chooseXbox { [self show:[HPXboxViewController new]]; }
- (BOOL)prefersStatusBarHidden { return YES; }
- (UIInterfaceOrientationMask)supportedInterfaceOrientations { return UIInterfaceOrientationMaskLandscape; }
@end

/* HALOPAD_ENGINE=pc or xbox skips the picker (development and tests) */
UIViewController *HPEngineChooserMake(UIViewController *(^makePC)(void))
{
	const char *engine = getenv("HALOPAD_ENGINE");
	HPEngineChooser *chooser;
	if (engine && !strcmp(engine, "pc"))
		return makePC();
	if (engine && !strcmp(engine, "xbox"))
		return [HPXboxViewController new];
	chooser = [HPEngineChooser new];
	chooser.makePC = makePC;
	return chooser;
}
