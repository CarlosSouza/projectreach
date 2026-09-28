#import <Foundation/Foundation.h>

/* Original files remain inert data. This service never loads executable code. */
NSString *HPGameDirectoryProblem(NSString *directory, NSString *expectedSHA256);
BOOL HPImportGameDirectory(NSURL *source, NSURL *destination, NSString *expectedSHA256,
                           NSURL **previousFolder, NSError **error);

/* Internal: publish a fully verified sibling staging directory. Never delete the old tree. */
BOOL HPPublishGameImport(NSString *stage, NSString *destination, NSURL **previousFolder, NSError **error);

/* trustedIdentity must come from the signed app bundle, never from the package. */
BOOL HPImportGamePackage(NSURL *archive, NSURL *destination, NSDictionary *trustedIdentity,
                         NSURL **previousFolder, NSError **error);
