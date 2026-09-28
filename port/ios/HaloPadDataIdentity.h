#import <Foundation/Foundation.h>

/* Shared folder/package policy; identity is supplied by the signed bundle. */
BOOL HPDataPathIsSafe(NSString *path);
NSString *HPDataPathKey(NSString *path);
BOOL HPDataPathsAreSafe(NSArray<NSString *> *paths);
NSDictionary *HPExpectedDataManifest(NSDictionary *identity);
