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
#import <GameController/GameController.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include "xg_ios.h"
#include "xg_xiso.h"
#import "HaloPadOverlay.h"
#import "HaloPadXboxSaveIdentity.h"
#include "xg_overlay_input.h"

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
	return development && *development ? @(development) : xbox_root();
}

static NSString *xbox_saves(void)
{
	const char *development = getenv("XG_SAVE");
	return development && *development ? @(development) : [xbox_root() stringByAppendingPathComponent:@"save"];
}

static BOOL xbox_has_maps(void)
{
	return [NSFileManager.defaultManager fileExistsAtPath:[xbox_data() stringByAppendingPathComponent:@"maps/ui.map"]];
}

static NSDictionary *xbox_build(void)
{
	NSData *data = [NSData dataWithContentsOfFile:[NSBundle.mainBundle.bundlePath stringByAppendingPathComponent:@"data/xbox/build.json"]];
	return data ? [NSJSONSerialization JSONObjectWithData:data options:0 error:nil] : @{};
}

/* Preserve a copy before a different guest opens snapshot saves. */
static BOOL xbox_backup_saves(NSError **error)
{
	/* Test saves must not update the real installation's revision marker. */
	const char *development = getenv("XG_SAVE");
	if (development && *development)
		return YES;
	NSString *revision = HPXboxSaveIdentity(xbox_build());
	NSString *previous = [NSUserDefaults.standardUserDefaults stringForKey:@"HaloPadXboxSaveRevision"];
	if (!revision.length)
	{
		if (error) *error = [NSError errorWithDomain:@"HaloPadXbox" code:1 userInfo:@{
			NSLocalizedDescriptionKey: @"The Xbox build has no guest identity. Rebuild before opening saves." }];
		return NO;
	}
	if ([revision isEqualToString:previous])
		return YES;
	NSFileManager *files = NSFileManager.defaultManager;
	NSString *saves = xbox_saves();
	NSArray *contents = [files fileExistsAtPath:saves] ? [files contentsOfDirectoryAtPath:saves error:error] : @[];
	if (!contents)
		return NO;
	if (contents.count)
	{
		NSString *backupRoot = [xbox_root() stringByAppendingPathComponent:@"Save Backups"];
		NSString *name = [NSString stringWithFormat:@"%@-%.0f", previous ?: @"unversioned", NSDate.date.timeIntervalSince1970];
		if (![files createDirectoryAtPath:backupRoot withIntermediateDirectories:YES attributes:nil error:error] ||
			![files copyItemAtPath:saves toPath:[backupRoot stringByAppendingPathComponent:name] error:error])
			return NO;
	}
	[NSUserDefaults.standardUserDefaults setObject:revision forKey:@"HaloPadXboxSaveRevision"];
	return YES;
}

/* ---------- the Xbox game */

@interface HPXboxViewController : UIViewController <UIDocumentPickerDelegate>
@property(nonatomic, copy) void (^returnToChooser)(void);
@end

@implementation HPXboxViewController
{
	UIView *game;
	HPOverlay *pad;
	struct xg_overlay_input touch_input;
	UIButton *link_button;
	UIView *import_panel;
	UILabel *import_status;
	UIProgressView *import_progress;
	UIButton *import_button;
	UIButton *back_button;
	BOOL started;
}

- (void)loadView
{
	UIView *root = [[UIView alloc] initWithFrame:UIScreen.mainScreen.bounds];
	root.backgroundColor = UIColor.blackColor;
	game = xg_ios_make_view(root.bounds);
	game.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
	[root addSubview:game];
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
	NSError *error = nil;
	if (!xbox_backup_saves(&error))
	{
		[self showProblem:[@"Your saves could not be backed up before this update. " stringByAppendingString:error.localizedDescription ?: @""]];
		return;
	}
	started = YES;
	back_button.hidden = YES;
	link_button.hidden = NO;
	import_panel.hidden = YES;
	/* The pad manages its own controller visibility. Create it only for the
	 * running game, so it cannot reappear or keep polling on the import screen. */
	if (!pad)
	{
		__weak HPXboxViewController *weak = self;
		pad = [[HPOverlay alloc] initWithFrame:self.view.bounds inputHandler:^(const hp_input *event) {
			HPXboxViewController *owner = weak;
			if (!owner) return;
			xg_overlay_event(&owner->touch_input, event);
			if (event->kind == HPI_CANCEL_TOUCH) xg_ios_clear_touch_pad();
			else if (event->kind == HPI_MOUSEMOVE) xg_ios_add_touch_look(event->dx, event->dy);
			else xg_ios_set_touch_pad(&owner->touch_input.pad);
		}];
		pad.analogMoveReady = YES;
		pad.inGame = YES;
		[pad setControllerLabel:@"A" hint:@"Xbox A. Select in menus." forControl:@"jump"];
		[pad setControllerLabel:@"B" hint:@"Xbox B. Back in menus." forControl:@"melee"];
		[pad setControllerLabel:@"X" hint:@"Xbox X. Use or reload with the default profile." forControl:@"action"];
		[pad setControllerLabel:@"X" hint:@"Xbox X. Use or reload with the default profile." forControl:@"reload"];
		[pad setControllerLabel:@"Y" hint:@"Xbox Y. Switch weapons or follow the menu prompt." forControl:@"switch"];
		pad.controllerConnected = ^BOOL {
			if (getenv("XG_TOUCH_SHOW")) return NO;
			for (GCController *controller in GCController.controllers) if (controller.extendedGamepad) return YES;
			return NO;
		};
		pad.engineMenuItems = @[
			[UIAction actionWithTitle:@"Xbox Controls…" image:[UIImage systemImageNamed:@"gamecontroller"] identifier:nil
				handler:^(__kindof UIAction *action) { [weak showControlsHelp]; }],
			[UIAction actionWithTitle:@"System Link…" image:[UIImage systemImageNamed:@"network"] identifier:nil
				handler:^(__kindof UIAction *action) { [weak showLink]; }]];
		[pad refreshControllerVisibility];
		pad.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
		[self.view insertSubview:pad belowSubview:link_button];
		link_button.hidden = YES;
	}
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

- (void)showControlsHelp
{
	[pad clearTouchInput];
	UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"Xbox Controls"
		message:@"Menus: use MOVE to highlight an item. Tap A (Jump) to select or B (Melee) to go back. X and Y follow the game's prompts.\n\nPlaying: use the same touch layout as PC. Drag the screen to aim, or drag FIRE while shooting.\n\nUse the Default Xbox control profile. Other in-game button layouts do not match these labels yet. Touch size, layout and sensitivity are in Controls."
		preferredStyle:UIAlertControllerStyleAlert];
	[alert addAction:[UIAlertAction actionWithTitle:@"Done" style:UIAlertActionStyleDefault handler:nil]];
	[self presentViewController:alert animated:YES completion:nil];
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
	back_button = [UIButton buttonWithType:UIButtonTypeSystem];
	[back_button setTitle:@"‹ Editions" forState:UIControlStateNormal];
	back_button.translatesAutoresizingMaskIntoConstraints = NO;
	back_button.hidden = !self.returnToChooser;
	[back_button addTarget:self action:@selector(backToChooser) forControlEvents:UIControlEventTouchUpInside];
	[self.view addSubview:back_button];
	link_button.hidden = YES;
	[NSLayoutConstraint activateConstraints:@[
		[back_button.leadingAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.leadingAnchor constant:20],
		[back_button.topAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor constant:8],
		[back_button.heightAnchor constraintGreaterThanOrEqualToConstant:44],
		[import_panel.centerXAnchor constraintEqualToAnchor:self.view.centerXAnchor],
		[import_panel.centerYAnchor constraintEqualToAnchor:self.view.centerYAnchor],
		[import_panel.widthAnchor constraintLessThanOrEqualToConstant:520],
		[import_panel.leadingAnchor constraintGreaterThanOrEqualToAnchor:self.view.safeAreaLayoutGuide.leadingAnchor constant:20],
		[import_panel.trailingAnchor constraintLessThanOrEqualToAnchor:self.view.safeAreaLayoutGuide.trailingAnchor constant:-20],
		[stack.topAnchor constraintEqualToAnchor:import_panel.topAnchor constant:28],
		[stack.bottomAnchor constraintEqualToAnchor:import_panel.bottomAnchor constant:-28],
		[stack.leadingAnchor constraintEqualToAnchor:import_panel.leadingAnchor constant:28],
		[stack.trailingAnchor constraintEqualToAnchor:import_panel.trailingAnchor constant:-28]]];
}

- (void)backToChooser
{
	if (!started && import_button.enabled && self.returnToChooser)
		self.returnToChooser();
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
	back_button.enabled = NO;
	import_progress.progress = 0;
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
			self->back_button.enabled = YES;
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
{
	UIStackView *cards;
	BOOL choosing;
}

- (UIButton *)cardWithTitle:(NSString *)title subtitle:(NSString *)subtitle symbol:(NSString *)symbol action:(SEL)action
{
	UIButtonConfiguration *configuration = [UIButtonConfiguration filledButtonConfiguration];
	UIButton *button;
	configuration.title = title;
	configuration.subtitle = subtitle;
	configuration.titleAlignment = UIButtonConfigurationTitleAlignmentLeading;
	configuration.baseBackgroundColor = [UIColor colorWithRed:0.065 green:0.105 blue:0.15 alpha:1];
	configuration.baseForegroundColor = UIColor.whiteColor;
	configuration.image = [UIImage systemImageNamed:symbol withConfiguration:[UIImageSymbolConfiguration configurationWithPointSize:32 weight:UIImageSymbolWeightRegular]];
	configuration.imagePlacement = NSDirectionalRectEdgeTop;
	configuration.imagePadding = 22;
	configuration.titlePadding = 12;
	configuration.cornerStyle = UIButtonConfigurationCornerStyleLarge;
	configuration.contentInsets = NSDirectionalEdgeInsetsMake(28, 24, 28, 24);
	configuration.titleTextAttributesTransformer = ^NSDictionary *(NSDictionary *in) {
		NSMutableDictionary *out = [in mutableCopy];
		out[NSFontAttributeName] = [UIFont preferredFontForTextStyle:UIFontTextStyleTitle1];
		return out;
	};
	configuration.subtitleTextAttributesTransformer = ^NSDictionary *(NSDictionary *in) {
		NSMutableDictionary *out = [in mutableCopy];
		out[NSFontAttributeName] = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
		out[NSForegroundColorAttributeName] = [UIColor colorWithWhite:0.85 alpha:1];
		return out;
	};
	button = [UIButton buttonWithConfiguration:configuration primaryAction:nil];
	button.contentHorizontalAlignment = UIControlContentHorizontalAlignmentLeading;
	button.titleLabel.numberOfLines = 0;
	button.subtitleLabel.numberOfLines = 0;
	button.titleLabel.adjustsFontForContentSizeCategory = YES;
	button.subtitleLabel.adjustsFontForContentSizeCategory = YES;
	button.layer.borderWidth = 1;
	button.layer.borderColor = [UIColor colorWithRed:0.19 green:0.31 blue:0.42 alpha:1].CGColor;
	button.accessibilityLabel = title;
	button.accessibilityValue = subtitle;
	button.accessibilityHint = @"Opens this edition of Halo";
	button.accessibilityIdentifier = action == @selector(choosePC) ? @"engine.pc" : @"engine.xbox";
	[button addTarget:self action:action forControlEvents:UIControlEventTouchUpInside];
	return button;
}

- (void)loadView
{
	UIView *root = [UIView new];
	UILabel *brand = [UILabel new], *title = [UILabel new], *note = [UILabel new];
	UIStackView *stack;
	root.backgroundColor = [UIColor colorWithRed:0.015 green:0.03 blue:0.05 alpha:1];
	brand.text = @"HALOPAD";
	brand.font = [UIFont systemFontOfSize:15 weight:UIFontWeightSemibold];
	brand.textColor = [UIColor colorWithRed:0.55 green:0.77 blue:0.94 alpha:1];
	brand.textAlignment = NSTextAlignmentCenter;
	title.text = @"Choose an edition";
	title.font = [UIFont preferredFontForTextStyle:UIFontTextStyleLargeTitle];
	title.adjustsFontForContentSizeCategory = YES;
	title.numberOfLines = 0;
	title.textColor = UIColor.whiteColor;
	title.textAlignment = NSTextAlignmentCenter;
	cards = [[UIStackView alloc] initWithArrangedSubviews:@[
		[self cardWithTitle:@"Halo Custom Edition" subtitle:@"WINDOWS • 1.10\nMultiplayer on PC community servers\n\nPlay Custom Edition →" symbol:@"desktopcomputer" action:@selector(choosePC)],
		[self cardWithTitle:@"Halo: Combat Evolved" subtitle:[NSString stringWithFormat:@"%@\nOriginal Xbox campaign\n\n%@ →", [xbox_build()[@"candidate"] boolValue] ? @"XBOX • PREVIEW" : @"XBOX • EXPERIMENTAL", xbox_has_maps() ? @"Play Xbox" : @"Add your Xbox disc"] symbol:@"gamecontroller" action:@selector(chooseXbox)] ]];
	cards.axis = UILayoutConstraintAxisHorizontal;
	cards.spacing = 24;
	cards.distribution = UIStackViewDistributionFillEqually;
	note.text = @"Each edition has its own saves and multiplayer. Reopen HaloPad to switch editions.";
	note.numberOfLines = 0;
	note.textAlignment = NSTextAlignmentCenter;
	note.textColor = [UIColor colorWithWhite:0.75 alpha:1];
	note.font = [UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];
	note.adjustsFontForContentSizeCategory = YES;
	UIButton *builds = [UIButton buttonWithType:UIButtonTypeSystem];
	[builds setTitle:@"About these builds" forState:UIControlStateNormal];
	builds.titleLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];
	builds.titleLabel.adjustsFontForContentSizeCategory = YES;
	[builds setTitleColor:brand.textColor forState:UIControlStateNormal];
	[builds.heightAnchor constraintGreaterThanOrEqualToConstant:44].active = YES;
	[builds addTarget:self action:@selector(showBuilds) forControlEvents:UIControlEventTouchUpInside];
	builds.accessibilityIdentifier = @"engine.builds";
	stack = [[UIStackView alloc] initWithArrangedSubviews:@[ brand, title, cards, note, builds ]];
	stack.axis = UILayoutConstraintAxisVertical;
	stack.spacing = 24;
	[stack setCustomSpacing:8 afterView:brand];
	[stack setCustomSpacing:4 afterView:note];
	stack.translatesAutoresizingMaskIntoConstraints = NO;
	UIScrollView *scroll = [UIScrollView new];
	UIView *content = [UIView new];
	scroll.translatesAutoresizingMaskIntoConstraints = NO;
	content.translatesAutoresizingMaskIntoConstraints = NO;
	[root addSubview:scroll];
	[scroll addSubview:content];
	[content addSubview:stack];
	[NSLayoutConstraint activateConstraints:@[
		[scroll.leadingAnchor constraintEqualToAnchor:root.safeAreaLayoutGuide.leadingAnchor],
		[scroll.trailingAnchor constraintEqualToAnchor:root.safeAreaLayoutGuide.trailingAnchor],
		[scroll.topAnchor constraintEqualToAnchor:root.safeAreaLayoutGuide.topAnchor],
		[scroll.bottomAnchor constraintEqualToAnchor:root.safeAreaLayoutGuide.bottomAnchor],
		[content.leadingAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.leadingAnchor],
		[content.trailingAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.trailingAnchor],
		[content.topAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.topAnchor],
		[content.bottomAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.bottomAnchor],
		[content.widthAnchor constraintEqualToAnchor:scroll.frameLayoutGuide.widthAnchor],
		[content.heightAnchor constraintGreaterThanOrEqualToAnchor:scroll.frameLayoutGuide.heightAnchor],
		[stack.centerXAnchor constraintEqualToAnchor:content.centerXAnchor],
		[stack.centerYAnchor constraintEqualToAnchor:content.centerYAnchor],
		[stack.topAnchor constraintGreaterThanOrEqualToAnchor:content.topAnchor constant:24],
		[stack.bottomAnchor constraintLessThanOrEqualToAnchor:content.bottomAnchor constant:-24],
		[stack.widthAnchor constraintLessThanOrEqualToConstant:960],
		[stack.leadingAnchor constraintGreaterThanOrEqualToAnchor:root.safeAreaLayoutGuide.leadingAnchor constant:24],
		[stack.trailingAnchor constraintLessThanOrEqualToAnchor:root.safeAreaLayoutGuide.trailingAnchor constant:-24]]];
	NSLayoutConstraint *height = [content.heightAnchor constraintEqualToAnchor:scroll.frameLayoutGuide.heightAnchor];
	height.priority = UILayoutPriorityDefaultLow;
	height.active = YES;
	self.view = root;
	NSLayoutConstraint *width = [stack.widthAnchor constraintEqualToAnchor:root.safeAreaLayoutGuide.widthAnchor constant:-64];
	width.priority = UILayoutPriorityDefaultHigh;
	width.active = YES;
	/* development: HALOPAD_CHOOSE=pc or xbox presses that card once the picker is up */
	if (getenv("HALOPAD_CHOOSE") && (!strcmp(getenv("HALOPAD_CHOOSE"), "pc") || !strcmp(getenv("HALOPAD_CHOOSE"), "xbox")))
	{
		UIButton *card = cards.arrangedSubviews[strcmp(getenv("HALOPAD_CHOOSE"), "xbox") ? 0 : 1];
		dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 2 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
			[card sendActionsForControlEvents:UIControlEventTouchUpInside];
		});
	}
}

- (void)viewDidLayoutSubviews
{
	[super viewDidLayoutSubviews];
	BOOL narrow = self.view.bounds.size.width < 650 || UIContentSizeCategoryIsAccessibilityCategory(self.traitCollection.preferredContentSizeCategory);
	cards.axis = narrow ? UILayoutConstraintAxisVertical : UILayoutConstraintAxisHorizontal;
}

- (void)showBuilds
{
	NSDictionary *build = xbox_build();
	NSString *revision = build[@"revision"] ?: @"unknown";
	NSString *message = [NSString stringWithFormat:@"Windows: Halo Custom Edition 1.10.\n\nXbox: halo-ce-universal %@ (built %@).%@\n\nThe Xbox port is experimental. Full campaign progression, split-screen and system link remain unverified in HaloPad. Xbox and Windows editions cannot play together.\n\nUpdates are validated on the Mac and iPad Simulator before the accepted pin moves. Saves are backed up when the engine changes.", [revision substringToIndex:MIN((NSUInteger)8, revision.length)], build[@"built"] ?: @"locally", [build[@"candidate"] boolValue] ? @"\nPreview candidate; validation is incomplete." : @""];
	UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"Installed builds" message:message preferredStyle:UIAlertControllerStyleAlert];
	[alert addAction:[UIAlertAction actionWithTitle:@"Done" style:UIAlertActionStyleCancel handler:nil]];
	[self presentViewController:alert animated:YES completion:nil];
}

- (void)show:(UIViewController *)controller
{
	if (choosing || !controller)
		return;
	choosing = YES;
	UIWindow *window = self.view.window;
	[NSUserDefaults.standardUserDefaults setObject:[controller isKindOfClass:HPXboxViewController.class] ? @"xbox" : @"pc" forKey:@"HaloPadLastEngine"];
	window.rootViewController = controller;
}

- (void)choosePC { [self show:self.makePC()]; }
- (void)chooseXbox
{
	HPXboxViewController *controller = [HPXboxViewController new];
	__weak UIWindow *window = self.view.window;
	controller.returnToChooser = ^{
		self->choosing = NO;
		window.rootViewController = self;
	};
	[self show:controller];
}
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
