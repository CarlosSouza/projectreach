/* Snapshot saves belong to the exact guest, not just its upstream commit. */
#import <Foundation/Foundation.h>

static inline NSString *HPXboxSaveIdentity(NSDictionary *build)
{
    NSString *revision = build[@"revision"];
    NSString *guest = build[@"guest_sha256"];
    if (![revision isKindOfClass:NSString.class] || !revision.length ||
        ![guest isKindOfClass:NSString.class] || guest.length != 64)
        return nil;
    return [NSString stringWithFormat:@"%@-%@", revision, guest];
}
