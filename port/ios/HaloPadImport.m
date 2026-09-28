#import "HaloPadImport.h"
#import "HaloPadDataIdentity.h"
#import <CommonCrypto/CommonDigest.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

/* Hold directory descriptors while resolving paths inside the selected root. A provider
   replacing a parent with a symlink cannot redirect subsequent file reads outside it. */
static int hp_open_file(NSString *root, NSString *relative)
{
    int directory = open(root.fileSystemRepresentation, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_NONBLOCK);
    if (directory < 0) return -1;
    NSArray *parts = [relative componentsSeparatedByString:@"/"];
    for (NSUInteger i = 0; i < parts.count; i++) {
        int flags = O_RDONLY | O_NOFOLLOW | O_NONBLOCK;
        if (i + 1 < parts.count) flags |= O_DIRECTORY;
        int next = openat(directory, [parts[i] fileSystemRepresentation], flags);
        int saved = errno;
        close(directory);
        directory = next;
        if (next < 0) { errno = saved; break; }
    }
    return directory;
}

/* Read regular files without following a final link. Stop at the original size if a
   provider changes the file while it is being hashed; validation then fails. */
static NSString *hp_hash(NSString *root, NSString *relative)
{
    int fd = hp_open_file(root, relative);
    struct stat before, after;
    if (fd < 0) return nil;
    if (fstat(fd, &before) || !S_ISREG(before.st_mode) || before.st_size < 0 || before.st_size > (INT64_C(2) << 30)) { close(fd); return nil; }
    CC_SHA256_CTX context; CC_SHA256_Init(&context);
    uint8_t buffer[65536]; uint64_t total = 0;
    BOOL ok = YES;
    for (;;) {
        ssize_t n = read(fd, buffer, sizeof buffer);
        if (n < 0 && errno == EINTR) continue;
        if (n < 0) { ok = NO; break; }
        if (!n) break;
        total += (uint64_t)n;
        if (total > (uint64_t)before.st_size) { ok = NO; break; }
        CC_SHA256_Update(&context, buffer, (CC_LONG)n);
    }
    ok &= !fstat(fd, &after) && total == (uint64_t)before.st_size && after.st_size == before.st_size &&
          after.st_mtimespec.tv_sec == before.st_mtimespec.tv_sec && after.st_mtimespec.tv_nsec == before.st_mtimespec.tv_nsec;
    close(fd);
    if (!ok) return nil;
    unsigned char digest[CC_SHA256_DIGEST_LENGTH]; CC_SHA256_Final(digest, &context);
    NSMutableString *result = [NSMutableString string];
    for (int i = 0; i < CC_SHA256_DIGEST_LENGTH; i++) [result appendFormat:@"%02x", digest[i]];
    return result;
}

/* Enumerate directories and files, including hidden entries. Bound traversal and use the
   same Unicode/case/path rules as ZIP import. Never descend into a symlink. */
static NSString *hp_scan(NSString *root, NSString *relative, NSMutableDictionary *files, NSUInteger *remaining, unsigned depth, unsigned maxDepth)
{
    if (depth > maxDepth) return @"The game folder is nested too deeply.";
    int fd = relative.length ? hp_open_file(root, relative) : open(root.fileSystemRepresentation, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return errno == ENOENT ? @"No Halo Custom Edition folder found. Choose your game data below." : @"The folder could not be read or contains a link.";
    DIR *entries = fdopendir(fd);
    if (!entries) { close(fd); return @"Choose a regular game folder."; }
    @try {
        NSMutableSet *seen = [NSMutableSet set];
        for (;;) {
            errno = 0;
            struct dirent *entry = readdir(entries);
            if (!entry) return errno ? @"The game folder could not be read." : nil;
            if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;
            if (!*remaining) return @"The folder contains too many entries. Choose the game's installation folder.";
            (*remaining)--;
            NSString *name = [NSFileManager.defaultManager stringWithFileSystemRepresentation:entry->d_name length:strlen(entry->d_name)];
            if (!name) return @"The folder has an unsupported filename.";
            NSString *rel = relative.length ? [relative stringByAppendingFormat:@"/%@", name] : name;
            NSString *key = HPDataPathKey(name);
            if (!HPDataPathIsSafe(rel) || [seen containsObject:key]) return @"The folder has unsafe or ambiguous filenames.";
            [seen addObject:key];
            struct stat info;
            if (fstatat(dirfd(entries), entry->d_name, &info, AT_SYMLINK_NOFOLLOW)) return @"A game file could not be read.";
            if (S_ISDIR(info.st_mode)) {
                NSString *problem = hp_scan(root, rel, files, remaining, depth + 1, maxDepth);
                if (problem) return problem;
            } else if (!S_ISREG(info.st_mode)) return @"The folder contains a link or unsupported file type.";
            else files[HPDataPathKey(rel)] = rel;
        }
    } @finally { closedir(entries); }
}

static NSString *hp_folder_problem(NSString *directory, NSDictionary *identity, BOOL exact, NSDictionary **sourceFiles)
{
    if (!HPExpectedDataManifest(identity)) return @"The app's prepared-data identity is missing or invalid. Rebuild the app first.";
    NSDictionary *stock = identity[@"stock_files"];
    NSMutableSet *parents = [NSMutableSet set];
    unsigned maxDepth = 32;
    for (NSString *name in stock) {
        maxDepth = MAX(maxDepth, (unsigned)[name componentsSeparatedByString:@"/"].count);
        NSString *parent = name.stringByDeletingLastPathComponent;
        while (parent.length) { [parents addObject:HPDataPathKey(parent)]; parent = parent.stringByDeletingLastPathComponent; }
    }
    /* Permit every signed inventory's implied directories as well as the file bound.
       Extra provider entries remain bounded, even for a shallow stock installation. */
    NSUInteger remaining = 4096 + parents.count;
    NSMutableDictionary *files = [NSMutableDictionary dictionary];
    NSString *problem = hp_scan(directory, @"", files, &remaining, 0, maxDepth);
    if (problem) return problem;
    for (NSString *name in stock) {
        NSString *relative = files[HPDataPathKey(name)];
        NSString *path = relative ? [directory stringByAppendingPathComponent:relative] : nil;
        NSDictionary *record = stock[name];
        struct stat info;
        if (!path || lstat(path.fileSystemRepresentation, &info) || !S_ISREG(info.st_mode) ||
            info.st_size < 0 || (uint64_t)info.st_size != [record[@"size"] unsignedLongLongValue] ||
            ![hp_hash(directory, relative) isEqual:record[@"sha256"]])
            return [NSString stringWithFormat:@"%@ is missing or differs from the supported game data. Choose a complete folder matching this app build.", name];
    }
    if (exact && files.count != stock.count)
        return @"This folder contains additional files. Choose it with Choose Folder to prepare a verified stock copy; your original folder will be kept as a backup.";
    if (sourceFiles) *sourceFiles = files;
    return nil;
}

NSString *HPGameDirectoryProblem(NSString *directory, NSDictionary *identity)
{
    return hp_folder_problem(directory, identity, YES, NULL);
}

static BOOL hp_fail(NSError **error, NSString *message)
{
    if (error) *error = [NSError errorWithDomain:@"HaloPadImport" code:1
                                      userInfo:@{NSLocalizedDescriptionKey:message}];
    return NO;
}

/* A growing source cannot turn a bounded approved file into an unbounded copy. The
   staged tree is hashed again before publication, catching same-size mutation as well. */
static BOOL hp_copy_file(NSString *root, NSString *relative, NSString *destination, uint64_t expected, NSError **error)
{
    int input = hp_open_file(root, relative);
    if (input < 0) return hp_fail(error, @"A source file changed or could not be opened.");
    struct stat info;
    if (fstat(input, &info) || !S_ISREG(info.st_mode) || info.st_size < 0 || (uint64_t)info.st_size != expected) {
        close(input); return hp_fail(error, @"A source file changed during import.");
    }
    int output = open(destination.fileSystemRepresentation, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0600);
    if (output < 0) { close(input); return hp_fail(error, @"Could not create an imported file. Check available storage."); }
    uint8_t bytes[65536]; uint64_t copied = 0; BOOL ok = YES;
    while (ok) {
        ssize_t n = read(input, bytes, (size_t)MIN(sizeof bytes, expected-copied+1));
        if (n < 0 && errno == EINTR) continue;
        if (n < 0 || (uint64_t)n > expected-copied) { ok = NO; break; }
        if (!n) { ok = copied == expected; break; }
        for (size_t at = 0; at < (size_t)n;) {
            ssize_t written = write(output, bytes+at, (size_t)n-at);
            if (written < 0 && errno == EINTR) continue;
            if (written <= 0) { ok = NO; break; }
            at += written;
        }
        copied += (uint64_t)n;
    }
    if (ok && fsync(output)) ok = NO;
    close(input);
    if (close(output)) ok = NO;
    return ok ? YES : hp_fail(error, @"The source changed or the copy could not be saved. Check available storage.");
}

/* Internal publication boundary shared by folder and ZIP import. The caller owns and
   has verified a unique sibling stage, and removes it only when this returns NO. */
BOOL HPPublishGameImport(NSString *stage, NSString *dst, NSURL **previous, NSError **error)
{
    struct stat target, staged;
    NSString *parent = dst.stringByDeletingLastPathComponent;
    if (![stage.stringByDeletingLastPathComponent isEqualToString:parent] ||
        [stage isEqualToString:dst] || lstat(stage.fileSystemRepresentation, &staged) || !S_ISDIR(staged.st_mode))
        return hp_fail(error, @"The import staging folder is invalid.");
    BOOL exists = lstat(dst.fileSystemRepresentation, &target) == 0;
    if ((!exists && errno != ENOENT) || (exists && !S_ISDIR(target.st_mode)))
        return hp_fail(error, @"The game destination is not a regular folder.");
    NSString *identifier = NSUUID.UUID.UUIDString;
    unsigned flags = exists ? RENAME_SWAP : RENAME_EXCL;
    if (renameatx_np(AT_FDCWD, stage.fileSystemRepresentation, AT_FDCWD, dst.fileSystemRepresentation, flags)) {
        int saved = errno;
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

/* The caller serializes imports. A sibling stage keeps publication on the same volume.
   Before publication, every failure leaves the destination untouched. On replacement,
   RENAME_SWAP keeps both complete trees even if the process stops at the commit boundary. */
static BOOL hp_import(NSURL *source, NSURL *destination, NSDictionary *identity,
                      NSURL **previous, NSError **error, NSFileManager *fm)
{
    if (previous) *previous = nil;
    if (error) *error = nil;
    if (!source.isFileURL || !destination.isFileURL) return hp_fail(error, @"Choose a local game folder.");
    NSDictionary *files = nil;
    NSString *problem = hp_folder_problem(source.path, identity, NO, &files);
    if (problem) return hp_fail(error, problem);
    NSString *src = source.URLByResolvingSymlinksInPath.standardizedURL.path;
    NSString *dst = destination.URLByResolvingSymlinksInPath.standardizedURL.path;
    struct stat target;
    BOOL exists = lstat(destination.path.fileSystemRepresentation, &target) == 0;
    if (exists && !S_ISDIR(target.st_mode)) return hp_fail(error, @"The game destination is not a regular folder.");
    if ([src isEqualToString:dst] && files.count == [identity[@"stock_files"] count]) return YES;
    if ([src hasPrefix:[dst stringByAppendingString:@"/"]] || [dst hasPrefix:[src stringByAppendingString:@"/"]])
        return hp_fail(error, @"Choose a game folder outside the current installation and its parent folder.");
    NSString *parent = dst.stringByDeletingLastPathComponent;
    NSString *identifier = NSUUID.UUID.UUIDString;
    NSString *stage = [parent stringByAppendingPathComponent:[@".halopad-import-" stringByAppendingString:identifier]];
    NSError *copyError = nil;
    BOOL copied = [fm createDirectoryAtPath:stage withIntermediateDirectories:NO attributes:nil error:&copyError];
    for (NSString *name in [[identity[@"stock_files"] allKeys] sortedArrayUsingSelector:@selector(compare:)]) {
        if (!copied) break;
        NSString *out = [stage stringByAppendingPathComponent:name];
        copied = [fm createDirectoryAtPath:out.stringByDeletingLastPathComponent withIntermediateDirectories:YES attributes:nil error:&copyError] &&
                 hp_copy_file(src, files[HPDataPathKey(name)], out, [identity[@"stock_files"][name][@"size"] unsignedLongLongValue], &copyError);
    }
    if (!copied) {
        [fm removeItemAtPath:stage error:nil];
        return hp_fail(error, [NSString stringWithFormat:@"The copy failed. Your existing files are unchanged. %@", copyError.localizedDescription ?: @""]);
    }
    problem = HPGameDirectoryProblem(stage, identity);
    if (problem) {
        [fm removeItemAtPath:stage error:nil];
        return hp_fail(error, [@"The copied folder could not be verified: " stringByAppendingString:problem]);
    }
    BOOL published = HPPublishGameImport(stage, dst, previous, error);
    if (!published) [fm removeItemAtPath:stage error:nil];
    return published;
}

BOOL HPImportGameDirectory(NSURL *source, NSURL *destination, NSDictionary *identity,
                           NSURL **previousFolder, NSError **error)
{
    return hp_import(source, destination, identity, previousFolder, error, NSFileManager.defaultManager);
}
