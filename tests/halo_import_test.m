/* Exercise the same Foundation importer used by the app, with disposable inert fixtures. */
#import <Foundation/Foundation.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
static int writeFailure, corruptCopy;
static NSString *growRoot, *redirectRoot, *outsideRoot;
static ssize_t test_write(int fd, const void *bytes, size_t length)
{
    if (growRoot) {
        for (NSString *name in [NSFileManager.defaultManager subpathsAtPath:growRoot]) {
            NSString *path = [growRoot stringByAppendingPathComponent:name];
            struct stat st;
            if (!lstat(path.fileSystemRepresentation, &st) && S_ISREG(st.st_mode)) {
                int sourceFD = open(path.fileSystemRepresentation, O_WRONLY | O_APPEND);
                assert(sourceFD >= 0);
                uint8_t extra[4096] = {0}; assert(write(sourceFD, extra, sizeof extra) == sizeof extra);
                close(sourceFD);
            }
        }
        growRoot = nil;
    }
    if (redirectRoot) {
        NSString *maps = [redirectRoot stringByAppendingPathComponent:@"maps"];
        NSString *saved = [redirectRoot.stringByDeletingLastPathComponent stringByAppendingPathComponent:@"saved-maps"];
        assert([NSFileManager.defaultManager moveItemAtPath:maps toPath:saved error:nil]);
        assert([NSFileManager.defaultManager createSymbolicLinkAtPath:maps withDestinationPath:outsideRoot error:nil]);
        redirectRoot = nil;
    }
    if (writeFailure && --writeFailure == 0) { errno = ENOSPC; return -1; }
    if (corruptCopy && length) {
        uint8_t modified[65536]; assert(length <= sizeof modified);
        memcpy(modified, bytes, length); modified[0] ^= 1;
        return write(fd, modified, length);
    }
    return write(fd, bytes, length);
}
static int renameFailure;
static int test_rename(int a, const char *b, int c, const char *d, unsigned flags)
{
    if (renameFailure && --renameFailure == 0) { errno = EIO; return -1; }
    return renameatx_np(a, b, c, d, flags);
}
#define write test_write
#define renameatx_np test_rename
#import "../port/ios/HaloPadImport.m"
#undef renameatx_np
#undef write

static NSFileManager *fm;
static NSString *root, *source, *destination;
static NSDictionary *identity;
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
    for (NSString *file in @[@"haloce.exe", @"strings.dll", @"keystone.dll", @"maps/ui.map", @"maps/bitmaps.map", @"maps/sounds.map", @"maps/loc.map", @"maps/bloodgulch.map", @"config.txt", @"shaders/test.bin"])
        put(source, file, @"inert test data");
    put(destination, @"keep.txt", @"previous installation and saves");
    NSMutableDictionary *stock = [NSMutableDictionary dictionary];
    for (NSString *name in [fm subpathsAtPath:source]) {
        NSString *path = [source stringByAppendingPathComponent:name];
        struct stat st; assert(!lstat(path.fileSystemRepresentation, &st));
        if (S_ISREG(st.st_mode)) stock[name] = @{@"size":@(st.st_size), @"sha256":hp_hash(source, name)};
    }
    NSString *zero = [@"" stringByPaddingToLength:64 withString:@"0" startingAtIndex:0];
    identity = @{@"schema":@1, @"id":zero, @"profile":@"fixture", @"stock_files":stock,
                 @"core_data":@{@"image.bin":@{@"size":@1, @"sha256":zero}}};
}
static void unchanged(void)
{
    assert([get(destination, @"keep.txt") isEqualToString:@"previous installation and saves"]);
    for (NSString *name in [fm contentsOfDirectoryAtPath:root error:nil]) assert(![name hasPrefix:@".halopad-import-"]);
}
static void rejected(void)
{
    NSError *error = nil;
    assert(!HPImportGameDirectory(url(source), url(destination), identity, NULL, &error));
    assert(error.localizedDescription.length);
    unchanged();
}
int main(void)
{
    @autoreleasepool {
        fm = NSFileManager.defaultManager;
        reset();
        assert([fm removeItemAtPath:destination error:nil]);
        assert(HPImportGameDirectory(url(source), url(destination), identity, NULL, NULL));
        assert(!HPGameDirectoryProblem(destination, identity));
        assert([get(source, @"haloce.exe") isEqualToString:@"inert test data"]);
        puts("PASS: first import publishes a complete valid copy and preserves source");
        struct stat before, after;
        assert(!lstat(destination.fileSystemRepresentation, &before));
        assert(HPImportGameDirectory(url(destination), url(destination), identity, NULL, NULL));
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
        assert(!HPImportGameDirectory(url(link), url(destination), identity, NULL, NULL)); unchanged();
        assert([fm removeItemAtPath:destination error:nil]);
        assert([fm createSymbolicLinkAtPath:destination withDestinationPath:source error:nil]);
        assert(!HPImportGameDirectory(url(source), url(destination), identity, NULL, NULL));
        assert([get(source, @"haloce.exe") isEqualToString:@"inert test data"]);
        puts("PASS: source and destination folder links are rejected");

        reset();
        assert(!HPImportGameDirectory(url(source), url([source stringByAppendingPathComponent:@"child"]), identity, NULL, NULL));
        assert(!HPImportGameDirectory(url(source), url(root), identity, NULL, NULL)); unchanged();
        puts("PASS: overlapping source/destination trees cannot recurse or replace their source");

        reset(); NSURL *backup = nil;
        assert(HPImportGameDirectory(url(source), url(destination), identity, &backup, NULL));
        assert(!HPGameDirectoryProblem(destination, identity));
        assert([get(backup.path, @"keep.txt") isEqualToString:@"previous installation and saves"]);
        puts("PASS: replacement atomically publishes new files and retains the previous tree");

        for (int mode = 1; mode <= 2; mode++) {
            reset(); writeFailure = mode == 1 ? 2 : 0; corruptCopy = mode == 2;
            NSError *error = nil;
            assert(!hp_import(url(source), url(destination), identity, NULL, &error, fm));
            assert(error.localizedDescription.length); unchanged();
            writeFailure = corruptCopy = 0;
        }
        puts("PASS: partial-copy failure and staged corruption preserve old files and clean staging");

        reset(); renameFailure = 1; rejected(); renameFailure = 0;
        puts("PASS: atomic publication failure preserves the old installation");
        reset(); renameFailure = 2; backup = nil;
        assert(HPImportGameDirectory(url(source), url(destination), identity, &backup, NULL));
        assert(!HPGameDirectoryProblem(destination, identity));
        assert([get(backup.path, @"keep.txt") isEqualToString:@"previous installation and saves"]);
        renameFailure = 0;
        puts("PASS: failed backup rename retains the previous tree and reports its actual location");

        reset();
        assert([fm moveItemAtPath:[source stringByAppendingPathComponent:@"haloce.exe"] toPath:[source stringByAppendingPathComponent:@"HALOCE.EXE"] error:nil]);
        assert(!HPGameDirectoryProblem(source, identity));
        assert(HPGameDirectoryProblem(source, nil));
        puts("PASS: Windows filename casing accepted and missing app profile rejected");
        for (NSString *name in @[@"strings.dll", @"maps/bloodgulch.map", @"config.txt", @"shaders/test.bin"]) {
            reset(); put(source, name, @"changed bytes!");
            assert(HPGameDirectoryProblem(source, identity)); rejected();
        }
        puts("PASS: same-size changes to maps, DLLs, configuration and shaders fail complete identity validation");

        reset(); put(source, @"unknown.dll", @"unsupported native input");
        put(source, @"player-save", @"source player's state");
        assert(HPGameDirectoryProblem(source, identity));
        backup = nil;
        assert(HPImportGameDirectory(url(source), url(destination), identity, &backup, NULL));
        assert(!HPGameDirectoryProblem(destination, identity));
        assert(![fm fileExistsAtPath:[destination stringByAppendingPathComponent:@"unknown.dll"]]);
        assert(![fm fileExistsAtPath:[destination stringByAppendingPathComponent:@"player-save"]]);
        assert([get(source, @"player-save") isEqual:@"source player's state"]);
        assert([get(backup.path, @"keep.txt") isEqual:@"previous installation and saves"]);
        puts("PASS: folder imports select only approved stock files; original extra files and previous saves survive");

        put(destination, @"legacy-installer.exe", @"inert old extra");
        assert(HPGameDirectoryProblem(destination, identity));
        backup = nil;
        assert(HPImportGameDirectory(url(destination), url(destination), identity, &backup, NULL));
        assert(backup && !HPGameDirectoryProblem(destination, identity));
        assert([get(backup.path, @"legacy-installer.exe") isEqual:@"inert old extra"]);
        puts("PASS: same-folder cleanup retains the entire original tree in a backup");

        reset();
        NSMutableDictionary *invalid = [identity mutableCopy];
        invalid[@"stock_files"] = @{@"../escape":identity[@"stock_files"][@"haloce.exe"]};
        assert(HPGameDirectoryProblem(source, invalid));
        assert(!HPImportGameDirectory(url(source), url(destination), invalid, NULL, NULL)); unchanged();
        put(source, @"trailing.", @"not a safe path"); rejected();
        reset();
        NSString *nested = @"";
        for (int i = 0; i < 34; i++) nested = [nested stringByAppendingPathComponent:@"deep"];
        put(source, [nested stringByAppendingPathComponent:@"x"], @"bounded traversal"); rejected();
        puts("PASS: invalid bundle inventory, unsafe names and excessive nesting fail closed");
        reset();
        NSString *approvedDeep = @"";
        for (int i = 0; i < 34; i++) approvedDeep = [approvedDeep stringByAppendingPathComponent:@"n"];
        approvedDeep = [approvedDeep stringByAppendingPathComponent:@"data"];
        put(source, approvedDeep, @"approved deep data");
        NSMutableDictionary *deepStock = [identity[@"stock_files"] mutableCopy];
        deepStock[approvedDeep] = @{@"size":@18, @"sha256":hp_hash(source, approvedDeep)};
        NSMutableDictionary *deepIdentity = [identity mutableCopy]; deepIdentity[@"stock_files"] = deepStock;
        assert(!HPGameDirectoryProblem(source, deepIdentity));
        assert(HPImportGameDirectory(url(source), url(destination), deepIdentity, NULL, NULL));
        assert(!HPGameDirectoryProblem(destination, deepIdentity));
        puts("PASS: directory bounds admit the full signed inventory, including approved deep paths");

        reset(); growRoot = source; rejected();
        puts("PASS: source growth after validation is bounded and cannot replace the installed tree");
        reset();
        outsideRoot = [root stringByAppendingPathComponent:@"outside-maps"];
        assert([fm copyItemAtPath:[source stringByAppendingPathComponent:@"maps"] toPath:outsideRoot error:nil]);
        redirectRoot = source; rejected();
        assert(hp_open_file(source, @"maps/ui.map") < 0);
        assert([get(outsideRoot, @"ui.map") isEqual:@"inert test data"]);
        puts("PASS: swapping a validated parent for a symlink cannot redirect copying, even to matching bytes");
        reset();
        for (int i = 0; i < 4097; i++) {
            NSString *path = [source stringByAppendingPathComponent:[NSString stringWithFormat:@"extra-%d", i]];
            int fd = open(path.fileSystemRepresentation, O_WRONLY | O_CREAT | O_EXCL, 0600);
            assert(fd >= 0); close(fd);
        }
        rejected();
        puts("PASS: excess directory entries stop enumeration without publishing any data");
        assert([fm removeItemAtPath:root error:nil]);
    }
    return 0;
}
