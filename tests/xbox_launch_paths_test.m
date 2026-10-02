/* Actual launch/save helpers, isolated filesystem/defaults and inert engine.
 * No window, game image, installation or real game-directory access. */
#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#include "../port/xbox/xg_ios.h"
#include "../port/xbox/xg_xiso.h"
#include <stdio.h>

static NSString *fixture_root;
static NSUserDefaults *fixture_defaults;
static BOOL fail_copy;
static int checks, failures;
static NSString *fixture_guest = @"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

static NSArray *fixture_search_paths(NSSearchPathDirectory directory, NSSearchPathDomainMask domain, BOOL expand)
{
    (void)domain; (void)expand;
    if (directory != NSDocumentDirectory) abort();
    return @[[fixture_root stringByAppendingPathComponent:@"Documents"]];
}
@interface HPFixtureBundle : NSObject
+ (instancetype)mainBundle;
- (NSString *)bundlePath;
@end
@implementation HPFixtureBundle
+ (instancetype)mainBundle { return [self new]; }
- (NSString *)bundlePath { return [fixture_root stringByAppendingPathComponent:@"Bundle"]; }
@end
@interface HPFixtureDefaults : NSObject
+ (NSUserDefaults *)standardUserDefaults;
@end
@implementation HPFixtureDefaults
+ (NSUserDefaults *)standardUserDefaults { return fixture_defaults; }
@end
@interface HPFixtureFiles : NSFileManager
@property(class, readonly, strong) HPFixtureFiles *defaultManager;
@end
@implementation HPFixtureFiles
+ (instancetype)defaultManager
{
    static HPFixtureFiles *files;
    if (!files) files = [self new];
    return files;
}
- (BOOL)copyItemAtPath:(NSString *)source toPath:(NSString *)destination error:(NSError **)error
{
    if (!fail_copy) return [super copyItemAtPath:source toPath:destination error:error];
    if (error) *error = [NSError errorWithDomain:NSCocoaErrorDomain code:NSFileWriteNoPermissionError userInfo:nil];
    return NO;
}
@end

#define NSSearchPathForDirectoriesInDomains fixture_search_paths
#define NSBundle HPFixtureBundle
#define NSUserDefaults HPFixtureDefaults
#define NSFileManager HPFixtureFiles
#include "../port/ios/HaloPadXbox.m"
#undef NSSearchPathForDirectoriesInDomains
#undef NSBundle
#undef NSUserDefaults
#undef NSFileManager

@implementation XGTouchPad
@end
/* Save helpers never construct gameplay UI; make an accidental call fatal. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincomplete-implementation"
#pragma clang diagnostic ignored "-Wobjc-property-implementation"
@implementation HPOverlay
+ (instancetype)alloc { abort(); }
@end
#pragma clang diagnostic pop
void xg_ios_set_touch_pad(const struct xg_touch_pad *state) { (void)state; abort(); }
void xg_ios_clear_touch_pad(void) { abort(); }
void xg_ios_add_touch_look(float dx, float dy) { (void)dx; (void)dy; abort(); }
UIView *xg_ios_make_view(CGRect frame) { return [[UIView alloc] initWithFrame:frame]; }
void xg_ios_view_resized(void) {}
int xg_ios_start(const char *image, const char *data, const char *save) { (void)image; (void)data; (void)save; abort(); }
int xg_extract_maps(const char *image, const char *root, xg_extract_progress progress, void *context,
                    char *build, size_t build_size, char *error, size_t error_size)
{
    (void)image; (void)root; (void)progress; (void)context; (void)build;
    (void)build_size; (void)error; (void)error_size; abort();
}
static void check(const char *label, BOOL passed)
{
    checks++; failures += !passed;
    printf("%s %s\n", passed ? "PASS" : "FAIL", label);
}
static void write_fixture(NSString *path, NSData *data)
{
    NSError *error = nil;
    if (![NSFileManager.defaultManager createDirectoryAtPath:path.stringByDeletingLastPathComponent
                withIntermediateDirectories:YES attributes:nil error:&error] ||
        ![data writeToFile:path options:NSDataWritingWithoutOverwriting error:&error]) {
        fprintf(stderr,"fixture write failed: %s\n",error.localizedDescription.UTF8String); abort();
    }
}
static NSArray *backups(void)
{
    return [NSFileManager.defaultManager contentsOfDirectoryAtPath:
        [xbox_root() stringByAppendingPathComponent:@"Save Backups"] error:nil] ?: @[];
}
static void revision(NSString *value)
{
    NSData *data = [NSJSONSerialization dataWithJSONObject:@{@"revision": value, @"guest_sha256": fixture_guest} options:0 error:nil];
    NSString *path = [fixture_root stringByAppendingPathComponent:@"Bundle/data/xbox/build.json"];
    if (![data writeToFile:path atomically:YES]) abort();
}
int main(int argc, char **argv)
{
    @autoreleasepool {
        if (argc != 2) return 2;
        fixture_root = @(argv[1]);
        if ([NSFileManager.defaultManager fileExistsAtPath:fixture_root]) return 2;
        fixture_defaults = [[NSUserDefaults alloc] initWithSuiteName:
            [@"dev.halopad.launch-test." stringByAppendingString:NSUUID.UUID.UUIDString]];
        NSString *normal = [fixture_root stringByAppendingPathComponent:@"Documents/Halo Xbox"];
        NSString *save = [normal stringByAppendingPathComponent:@"save"];
        NSData *profile = [@"synthetic save, not game data" dataUsingEncoding:NSUTF8StringEncoding];
        write_fixture([normal stringByAppendingPathComponent:@"maps/ui.map"], [NSData data]);
        write_fixture([save stringByAppendingPathComponent:@"profile.bin"], profile);
        write_fixture([fixture_root stringByAppendingPathComponent:@"Bundle/data/xbox/build.json"],
            [NSJSONSerialization dataWithJSONObject:@{@"revision": @"new-pin", @"guest_sha256": fixture_guest} options:0 error:nil]);
        NSString *newIdentity = HPXboxSaveIdentity(xbox_build());
        unsetenv("XG_DATA"); unsetenv("XG_SAVE");
        check("unset data selects the ordinary installation", [xbox_data() isEqualToString:normal]);
        check("unset save selects ordinary saves", [xbox_saves() isEqualToString:save]);
        check("ordinary ui.map makes Xbox ready", xbox_has_maps());
        setenv("XG_DATA", "", 1); setenv("XG_SAVE", "", 1);
        check("empty data selects the ordinary installation", [xbox_data() isEqualToString:normal]);
        check("empty save selects ordinary saves", [xbox_saves() isEqualToString:save]);
        check("empty overrides do not hide installed maps", xbox_has_maps());
        [fixture_defaults setObject:@"old-pin" forKey:@"HaloPadXboxSaveRevision"];
        NSError *error = nil;
        check("empty save override still performs the revision backup", xbox_backup_saves(&error) && backups().count == 1);
        NSString *copy = [[[normal stringByAppendingPathComponent:@"Save Backups"]
            stringByAppendingPathComponent:backups().firstObject ?: @"missing"] stringByAppendingPathComponent:@"profile.bin"];
        check("backup preserves synthetic profile bytes", [[NSData dataWithContentsOfFile:copy] isEqualToData:profile]);
        check("backup never changes current synthetic save", [[NSData dataWithContentsOfFile:[save stringByAppendingPathComponent:@"profile.bin"]] isEqualToData:profile]);
        check("successful backup upgrades legacy marker to exact guest", [[fixture_defaults stringForKey:@"HaloPadXboxSaveRevision"] isEqualToString:newIdentity]);
        check("same revision does not create another backup", xbox_backup_saves(&error) && backups().count == 1);

        NSString *development = [fixture_root stringByAppendingPathComponent:@"development"];
        setenv("XG_DATA", development.fileSystemRepresentation, 1);
        setenv("XG_SAVE", [development stringByAppendingPathComponent:@"saves"].fileSystemRepresentation, 1);
        check("nonempty data override remains supported", [xbox_data() isEqualToString:development]);
        check("nonempty save override remains supported", [xbox_saves() isEqualToString:[development stringByAppendingPathComponent:@"saves"]]);
        check("development data never borrows normal maps", !xbox_has_maps());
        revision(@"later-pin");
        check("isolated saves skip real-installation backups", xbox_backup_saves(&error) && backups().count == 1);
        check("isolated saves do not advance the real revision marker", [[fixture_defaults stringForKey:@"HaloPadXboxSaveRevision"] isEqualToString:newIdentity]);

        unsetenv("XG_DATA"); unsetenv("XG_SAVE");
        fail_copy = YES; error = nil;
        check("copy failure refuses revision acceptance", !xbox_backup_saves(&error) && error != nil);
        check("copy failure leaves the revision marker unchanged", [[fixture_defaults stringForKey:@"HaloPadXboxSaveRevision"] isEqualToString:newIdentity]);
        check("copy failure retains current saves and old backup", backups().count == 1 &&
            [[NSData dataWithContentsOfFile:[save stringByAppendingPathComponent:@"profile.bin"]] isEqualToData:profile] &&
            [[NSData dataWithContentsOfFile:copy] isEqualToData:profile]);
        fail_copy = NO; error = nil;
        check("retry backs up before advancing revision", xbox_backup_saves(&error) && backups().count == 2 &&
            [[fixture_defaults stringForKey:@"HaloPadXboxSaveRevision"] isEqualToString:HPXboxSaveIdentity(xbox_build())]);
        fixture_guest = @"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
        revision(@"later-pin");
        check("same upstream pin with adapted guest creates another backup", xbox_backup_saves(&error) && backups().count == 3);
        check("adapted guest marker is exact", [[fixture_defaults stringForKey:@"HaloPadXboxSaveRevision"] isEqualToString:HPXboxSaveIdentity(xbox_build())]);
        check("same adapted guest does not repeat backup", xbox_backup_saves(&error) && backups().count == 3);
        fixture_guest = @"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
        revision(@"later-pin");
        check("returning to unadapted guest also backs up", xbox_backup_saves(&error) && backups().count == 4);
        NSString *accepted = [fixture_defaults stringForKey:@"HaloPadXboxSaveRevision"];
        fixture_guest = @""; revision(@"later-pin"); error = nil;
        check("missing guest identity refuses opening saves", !xbox_backup_saves(&error) && error != nil && backups().count == 4);
        check("invalid identity leaves marker and current save untouched", [[fixture_defaults stringForKey:@"HaloPadXboxSaveRevision"] isEqualToString:accepted] &&
            [[NSData dataWithContentsOfFile:[save stringByAppendingPathComponent:@"profile.bin"]] isEqualToData:profile]);
        printf("%d checks, %d failures\n",checks,failures);
    }
    return failures != 0;
}
