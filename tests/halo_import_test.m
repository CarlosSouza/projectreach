/* Exercise the same Foundation importer used by the app, with disposable inert fixtures. */
#import <Foundation/Foundation.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
static int renameFailure;
static int test_rename(int a, const char *b, int c, const char *d, unsigned flags)
{
    if (renameFailure && --renameFailure == 0) { errno = EIO; return -1; }
    return renameatx_np(a, b, c, d, flags);
}
#define renameatx_np test_rename
#import "../port/ios/HaloPadImport.m"
#undef renameatx_np

@interface BrokenCopy : NSFileManager
@property int mode;
@end
@implementation BrokenCopy
- (BOOL)copyItemAtPath:(NSString *)source toPath:(NSString *)destination error:(NSError **)error
{
    BOOL result = [super copyItemAtPath:source toPath:destination error:error];
    assert(result);
    if (self.mode == 1) {
        if (error) *error = [NSError errorWithDomain:NSPOSIXErrorDomain code:ENOSPC userInfo:nil];
        return NO;
    }
    [NSData.data writeToFile:[destination stringByAppendingPathComponent:@"maps/ui.map"] atomically:YES];
    return YES;
}
@end

static NSFileManager *fm;
static NSString *root, *source, *destination, *hash;
static NSURL *url(NSString *s) { return [NSURL fileURLWithPath:s]; }
static void put(NSString *base, NSString *relative, NSString *value)
{
    NSString *path = [base stringByAppendingPathComponent:relative];
    assert([fm createDirectoryAtPath:path.stringByDeletingLastPathComponent withIntermediateDirectories:YES attributes:nil error:nil]);
    assert([value writeToFile:path atomically:YES encoding:NSUTF8StringEncoding error:nil]);
}
static NSString *get(NSString *base, NSString *relative)
{
    return [NSString stringWithContentsOfFile:[base stringByAppendingPathComponent:relative] encoding:NSUTF8StringEncoding error:nil];
}
static void reset(void)
{
    if (root) assert([fm removeItemAtPath:root error:nil]);
    root = [NSTemporaryDirectory() stringByAppendingPathComponent:[@"halopad-import-test-" stringByAppendingString:NSUUID.UUID.UUIDString]];
    source = [root stringByAppendingPathComponent:@"source"];
    destination = [root stringByAppendingPathComponent:@"Halo Custom Edition"];
    for (NSString *file in @[@"haloce.exe", @"strings.dll", @"keystone.dll", @"maps/ui.map", @"maps/bitmaps.map", @"maps/sounds.map", @"maps/loc.map", @"maps/bloodgulch.map"])
        put(source, file, @"inert test data");
    put(destination, @"keep.txt", @"previous installation and saves");
    hash = hp_hash([source stringByAppendingPathComponent:@"haloce.exe"]);
}
static void unchanged(void)
{
    assert([get(destination, @"keep.txt") isEqualToString:@"previous installation and saves"]);
    for (NSString *name in [fm contentsOfDirectoryAtPath:root error:nil]) assert(![name hasPrefix:@".halopad-import-"]);
}
static void rejected(void)
{
    NSError *error = nil;
    assert(!HPImportGameDirectory(url(source), url(destination), hash, NULL, &error));
    assert(error.localizedDescription.length);
    unchanged();
}
int main(void)
{
    @autoreleasepool {
        fm = NSFileManager.defaultManager;
        reset();
        assert([fm removeItemAtPath:destination error:nil]);
        assert(HPImportGameDirectory(url(source), url(destination), hash, NULL, NULL));
        assert(!HPGameDirectoryProblem(destination, hash));
        assert([get(source, @"haloce.exe") isEqualToString:@"inert test data"]);
        puts("PASS: first import publishes a complete valid copy and preserves source");
        struct stat before, after;
        assert(!lstat(destination.fileSystemRepresentation, &before));
        assert(HPImportGameDirectory(url(destination), url(destination), hash, NULL, NULL));
        assert(!lstat(destination.fileSystemRepresentation, &after) && before.st_ino == after.st_ino);
        puts("PASS: selecting the installed folder validates without replacing or deleting it");

        reset(); put(source, @"haloce.exe", @"wrong executable"); rejected();
        puts("PASS: wrong executable preserves previous installation");
        reset(); assert([fm removeItemAtPath:[source stringByAppendingPathComponent:@"maps/ui.map"] error:nil]); rejected();
        reset(); put(source, @"maps/ui.map", @""); rejected();
        reset(); assert([fm removeItemAtPath:[source stringByAppendingPathComponent:@"maps/ui.map"] error:nil]);
        assert([fm createDirectoryAtPath:[source stringByAppendingPathComponent:@"maps/ui.map"] withIntermediateDirectories:YES attributes:nil error:nil]); rejected();
        puts("PASS: missing, empty and directory-valued required resources are rejected");

        reset(); NSString *outside = [root stringByAppendingPathComponent:@"outside"];
        put(outside, @"secret", @"keep outside");
        assert([fm createSymbolicLinkAtPath:[source stringByAppendingPathComponent:@"linked"] withDestinationPath:outside error:nil]); rejected();
        assert([get(outside, @"secret") isEqualToString:@"keep outside"]);
        reset(); assert([fm createSymbolicLinkAtPath:[source stringByAppendingPathComponent:@"dangling"] withDestinationPath:@"missing" error:nil]); rejected();
        reset(); assert(!mkfifo([source stringByAppendingPathComponent:@"pipe"].fileSystemRepresentation, 0600)); rejected();
        puts("PASS: external/dangling links and special files rejected without traversal");
        reset(); NSString *link = [root stringByAppendingPathComponent:@"link"];
        assert([fm createSymbolicLinkAtPath:link withDestinationPath:source error:nil]);
        assert(!HPImportGameDirectory(url(link), url(destination), hash, NULL, NULL)); unchanged();
        assert([fm removeItemAtPath:destination error:nil]);
        assert([fm createSymbolicLinkAtPath:destination withDestinationPath:source error:nil]);
        assert(!HPImportGameDirectory(url(source), url(destination), hash, NULL, NULL));
        assert([get(source, @"haloce.exe") isEqualToString:@"inert test data"]);
        puts("PASS: source and destination folder links are rejected");

        reset();
        assert(!HPImportGameDirectory(url(source), url([source stringByAppendingPathComponent:@"child"]), hash, NULL, NULL));
        assert(!HPImportGameDirectory(url(source), url(root), hash, NULL, NULL)); unchanged();
        puts("PASS: overlapping source/destination trees cannot recurse or replace their source");

        reset(); NSURL *backup = nil;
        assert(HPImportGameDirectory(url(source), url(destination), hash, &backup, NULL));
        assert(!HPGameDirectoryProblem(destination, hash));
        assert([get(backup.path, @"keep.txt") isEqualToString:@"previous installation and saves"]);
        puts("PASS: replacement atomically publishes new files and retains the previous tree");

        for (int mode = 1; mode <= 2; mode++) {
            reset(); BrokenCopy *broken = [BrokenCopy new]; broken.mode = mode;
            NSError *error = nil;
            assert(!hp_import(url(source), url(destination), hash, NULL, &error, broken));
            assert(error.localizedDescription.length); unchanged();
        }
        puts("PASS: partial-copy failure and staged corruption preserve old files and clean staging");

        reset(); renameFailure = 1; rejected(); renameFailure = 0;
        puts("PASS: atomic publication failure preserves the old installation");
        reset(); renameFailure = 2; backup = nil;
        assert(HPImportGameDirectory(url(source), url(destination), hash, &backup, NULL));
        assert(!HPGameDirectoryProblem(destination, hash));
        assert([get(backup.path, @"keep.txt") isEqualToString:@"previous installation and saves"]);
        renameFailure = 0;
        puts("PASS: failed backup rename retains the previous tree and reports its actual location");

        reset();
        assert([fm moveItemAtPath:[source stringByAppendingPathComponent:@"haloce.exe"] toPath:[source stringByAppendingPathComponent:@"HALOCE.EXE"] error:nil]);
        assert(!HPGameDirectoryProblem(source, hash));
        assert(HPGameDirectoryProblem(source, nil));
        puts("PASS: Windows filename casing accepted and missing app profile rejected");
        assert([fm removeItemAtPath:root error:nil]);
    }
    return 0;
}
