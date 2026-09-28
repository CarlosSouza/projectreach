#import <Foundation/Foundation.h>

/* Original files remain inert data. This service never loads executable code. */
NSString *HPGameDirectoryProblem(NSString *directory, NSString *expectedSHA256);
BOOL HPImportGameDirectory(NSURL *source, NSURL *destination, NSString *expectedSHA256,
                           NSURL **previousFolder, NSError **error);
