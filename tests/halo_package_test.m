/* Command-line adapter for the real Foundation package importer; never starts a game. */
#import <Foundation/Foundation.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
static int failRename, failWrite;
static int package_rename(int a, const char *b, int c, const char *d, unsigned flags)
{
    if (failRename && --failRename == 0) { errno = EIO; return -1; }
    return renameatx_np(a, b, c, d, flags);
}
static ssize_t package_write(int fd, const void *p, size_t n)
{
    if (failWrite && --failWrite == 0) { errno = ENOSPC; return -1; }
    return write(fd, p, n);
}
#define renameatx_np package_rename
#import "../port/ios/HaloPadImport.m"
#undef renameatx_np
#define write package_write
#import "../port/ios/HaloPadPackage.m"
#undef write
int main(int argc, const char **argv)
{
    @autoreleasepool {
        if (argc < 4) return 2;
        if (argc > 4) {
            failRename = !strcmp(argv[4], "publish") ? 1 : !strcmp(argv[4], "backup") ? 2 : 0;
            failWrite = !strcmp(argv[4], "write-first") ? 1 : !strcmp(argv[4], "write") ? 2 : 0;
        }
        NSDictionary *identity = [NSJSONSerialization JSONObjectWithData:[NSData dataWithContentsOfFile:@(argv[1])] ?: NSData.data options:0 error:nil];
        __block NSError *error = nil;
        __block NSURL *backup = nil;
        __block BOOL ok = NO;
        dispatch_semaphore_t finished = dispatch_semaphore_create(0);
        dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
            @autoreleasepool {
                if (argc > 4 && !strcmp(argv[4], "folder"))
                    ok = HPImportGameDirectory([NSURL fileURLWithPath:@(argv[2])], [NSURL fileURLWithPath:@(argv[3])], identity, &backup, &error);
                else if (argc > 4 && !strcmp(argv[4], "validate")) {
                    NSString *problem = HPGameDirectoryProblem(@(argv[2]), identity);
                    ok = !problem;
                    if (problem) error = [NSError errorWithDomain:@"FolderTest" code:1 userInfo:@{NSLocalizedDescriptionKey:problem}];
                } else ok = HPImportGamePackage([NSURL fileURLWithPath:@(argv[2])], [NSURL fileURLWithPath:@(argv[3])], identity, &backup, &error);
            }
            dispatch_semaphore_signal(finished);
        });
        dispatch_semaphore_wait(finished, DISPATCH_TIME_FOREVER);
        NSData *result = [NSJSONSerialization dataWithJSONObject:@{@"ok":@(ok), @"error":error.localizedDescription ?: @"", @"backup":backup.path ?: @""} options:0 error:nil];
        puts([[NSString alloc] initWithData:result encoding:NSUTF8StringEncoding].UTF8String);
        return ok ? 0 : 1;
    }
}
