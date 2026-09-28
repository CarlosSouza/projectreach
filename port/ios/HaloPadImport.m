#import "HaloPadImport.h"
#import <CommonCrypto/CommonDigest.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>

static NSString *hp_hash(NSString *path)
{
    NSInputStream *stream = [NSInputStream inputStreamWithFileAtPath:path];
    [stream open];
    CC_SHA256_CTX context; CC_SHA256_Init(&context);
    uint8_t buffer[65536]; NSInteger n;
    while ((n = [stream read:buffer maxLength:sizeof buffer]) > 0)
        CC_SHA256_Update(&context, buffer, (CC_LONG)n);
    [stream close];
    if (n < 0) return nil;
    unsigned char digest[CC_SHA256_DIGEST_LENGTH]; CC_SHA256_Final(digest, &context);
    NSMutableString *result = [NSMutableString string];
    for (int i = 0; i < CC_SHA256_DIGEST_LENGTH; i++) [result appendFormat:@"%02x", digest[i]];
    return result;
}

/* Inspect every entry, including hidden ones. Do not traverse symlinks. Case collisions
   would make Windows path lookup ambiguous on a case-sensitive provider/device. */
static NSString *hp_tree_problem(NSString *directory)
{
    struct stat info;
    if (lstat(directory.fileSystemRepresentation, &info))
        return errno == ENOENT ? @"No Halo Custom Edition folder found. Choose your game folder below." : @"The folder could not be read.";
    if (!S_ISDIR(info.st_mode)) return @"Choose a regular folder, not a link or file.";
    NSError *error = nil;
    NSArray *names = [NSFileManager.defaultManager contentsOfDirectoryAtPath:directory error:&error];
    if (!names) return [NSString stringWithFormat:@"Cannot read the folder: %@", error.localizedDescription];
    NSMutableSet *seen = [NSMutableSet set];
    for (NSString *name in names) {
        NSString *key = name.precomposedStringWithCanonicalMapping.lowercaseString;
        if ([seen containsObject:key]) return [NSString stringWithFormat:@"The folder has ambiguous filenames: %@.", name];
        [seen addObject:key];
        NSString *path = [directory stringByAppendingPathComponent:name];
        if (lstat(path.fileSystemRepresentation, &info)) return [NSString stringWithFormat:@"Cannot read %@.", name];
        if (S_ISDIR(info.st_mode)) {
            NSString *problem = hp_tree_problem(path);
            if (problem) return problem;
        } else if (!S_ISREG(info.st_mode)) return [NSString stringWithFormat:@"%@ is a link or unsupported file type.", name];
    }
    return nil;
}

static NSString *hp_path_ci(NSString *directory, NSString *relative)
{
    NSString *path = directory;
    for (NSString *part in [relative componentsSeparatedByString:@"/"]) {
        NSString *hit = nil;
        for (NSString *name in [NSFileManager.defaultManager contentsOfDirectoryAtPath:path error:nil])
            if ([name caseInsensitiveCompare:part] == NSOrderedSame) { hit = name; break; }
        if (!hit) return nil;
        path = [path stringByAppendingPathComponent:hit];
    }
    return path;
}

NSString *HPGameDirectoryProblem(NSString *directory, NSString *expectedSHA256)
{
    if (expectedSHA256.length != 64) return @"The app's game profile is missing or invalid.";
    NSString *problem = hp_tree_problem(directory);
    if (problem) return problem;
    for (NSString *relative in @[@"haloce.exe", @"strings.dll", @"keystone.dll", @"maps/ui.map",
                                @"maps/bitmaps.map", @"maps/sounds.map", @"maps/loc.map", @"maps/bloodgulch.map"]) {
        NSString *path = hp_path_ci(directory, relative);
        struct stat info;
        if (!path || lstat(path.fileSystemRepresentation, &info) || !S_ISREG(info.st_mode) || info.st_size == 0)
            return [NSString stringWithFormat:@"The folder needs a nonempty %@ file.", relative];
    }
    if (![hp_hash(hp_path_ci(directory, @"haloce.exe")) isEqualToString:expectedSHA256])
        return @"haloce.exe does not match the supported Custom Edition 1.10 version. Install the official 1.10 update first.";
    return nil;
}

static BOOL hp_fail(NSError **error, NSString *message)
{
    if (error) *error = [NSError errorWithDomain:@"HaloPadImport" code:1
                                      userInfo:@{NSLocalizedDescriptionKey:message}];
    return NO;
}

/* The caller serializes imports. A sibling stage keeps publication on the same volume.
   Before publication, every failure leaves the destination untouched. On replacement,
   RENAME_SWAP keeps both complete trees even if the process stops at the commit boundary. */
static BOOL hp_import(NSURL *source, NSURL *destination, NSString *hash,
                      NSURL **previous, NSError **error, NSFileManager *fm)
{
    if (previous) *previous = nil;
    if (error) *error = nil;
    if (!source.isFileURL || !destination.isFileURL) return hp_fail(error, @"Choose a local game folder.");
    NSString *problem = HPGameDirectoryProblem(source.path, hash);
    if (problem) return hp_fail(error, problem);
    NSString *src = source.URLByResolvingSymlinksInPath.standardizedURL.path;
    NSString *dst = destination.URLByResolvingSymlinksInPath.standardizedURL.path;
    struct stat target;
    BOOL exists = lstat(destination.path.fileSystemRepresentation, &target) == 0;
    if (exists && !S_ISDIR(target.st_mode)) return hp_fail(error, @"The game destination is not a regular folder.");
    if ([src isEqualToString:dst]) return YES; /* Selecting the installed folder is validation only. */
    if ([src hasPrefix:[dst stringByAppendingString:@"/"]] || [dst hasPrefix:[src stringByAppendingString:@"/"]])
        return hp_fail(error, @"Choose a game folder outside the current installation and its parent folder.");
    NSString *parent = dst.stringByDeletingLastPathComponent;
    NSString *identifier = NSUUID.UUID.UUIDString;
    NSString *stage = [parent stringByAppendingPathComponent:[@".halopad-import-" stringByAppendingString:identifier]];
    NSError *copyError = nil;
    if (![fm copyItemAtPath:src toPath:stage error:&copyError]) {
        [fm removeItemAtPath:stage error:nil];
        return hp_fail(error, [NSString stringWithFormat:@"The copy failed. Your existing files are unchanged. %@", copyError.localizedDescription]);
    }
    problem = HPGameDirectoryProblem(stage, hash);
    if (problem) {
        [fm removeItemAtPath:stage error:nil];
        return hp_fail(error, [@"The copied folder could not be verified: " stringByAppendingString:problem]);
    }
    unsigned flags = exists ? RENAME_SWAP : RENAME_EXCL;
    if (renameatx_np(AT_FDCWD, stage.fileSystemRepresentation, AT_FDCWD, dst.fileSystemRepresentation, flags)) {
        int saved = errno;
        [fm removeItemAtPath:stage error:nil];
        return hp_fail(error, [NSString stringWithFormat:@"Could not install the copied folder. Your existing files are unchanged. %@",
                              [NSError errorWithDomain:NSPOSIXErrorDomain code:saved userInfo:nil].localizedDescription]);
    }
    if (exists) {
        NSString *backup = [parent stringByAppendingPathComponent:[@"Halo Custom Edition Backup " stringByAppendingString:identifier]];
        /* A failed cosmetic rename must never remove the old tree; expose its retained path. */
        if (renameatx_np(AT_FDCWD, stage.fileSystemRepresentation, AT_FDCWD, backup.fileSystemRepresentation, RENAME_EXCL)) backup = stage;
        if (previous) *previous = [NSURL fileURLWithPath:backup isDirectory:YES];
    }
    return YES;
}

BOOL HPImportGameDirectory(NSURL *source, NSURL *destination, NSString *hash,
                           NSURL **previousFolder, NSError **error)
{
    return hp_import(source, destination, hash, previousFolder, error, NSFileManager.defaultManager);
}
