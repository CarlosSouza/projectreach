#import "HaloPadDataIdentity.h"
#define HP_MAX_FILE (UINT64_C(2) << 30)
#define HP_MAX_TOTAL (UINT64_C(8) << 30)

BOOL HPDataPathIsSafe(NSString *name)
{
    if (![name isKindOfClass:NSString.class] || !name.length || [name lengthOfBytesUsingEncoding:NSUTF8StringEncoding] > 512) return NO;
    for (NSUInteger i = 0; i < name.length; i++) {
        unichar c = [name characterAtIndex:i];
        if (c < 32 || c == 127 || c == '\\' || c == ':') return NO;
    }
    for (NSString *part in [name componentsSeparatedByString:@"/"])
        if (!part.length || [part isEqual:@"."] || [part isEqual:@".."] || [part hasSuffix:@"."] || [part hasSuffix:@" "]) return NO;
    return YES;
}
NSString *HPDataPathKey(NSString *s)
{
    return [s.precomposedStringWithCanonicalMapping stringByFoldingWithOptions:NSCaseInsensitiveSearch locale:[NSLocale localeWithLocaleIdentifier:@"en_US_POSIX"]];
}
BOOL HPDataPathsAreSafe(NSArray<NSString *> *names)
{
    NSMutableDictionary *prefixes = [NSMutableDictionary dictionary];
    NSMutableSet *files = [NSMutableSet set];
    for (NSString *name in names) {
        if (!HPDataPathIsSafe(name) || [files containsObject:HPDataPathKey(name)]) return NO;
        [files addObject:HPDataPathKey(name)];
        NSString *prefix = @"";
        for (NSString *part in [name componentsSeparatedByString:@"/"]) {
            prefix = prefix.length ? [prefix stringByAppendingFormat:@"/%@", part] : part;
            NSString *key = HPDataPathKey(prefix);
            if (prefixes[key] && ![prefixes[key] isEqual:prefix]) return NO;
            prefixes[key] = prefix;
        }
    }
    for (NSString *name in names) {
        NSString *parent = name.stringByDeletingLastPathComponent;
        while (parent.length) {
            if ([files containsObject:HPDataPathKey(parent)]) return NO;
            parent = parent.stringByDeletingLastPathComponent;
        }
    }
    return YES;
}
static BOOL integer(id n)
{
    return [n isKindOfClass:NSNumber.class] && CFGetTypeID((__bridge CFTypeRef)n) != CFBooleanGetTypeID() &&
           strchr("cCsSiIlLqQ", [(NSNumber *)n objCType][0]) != NULL;
}
static BOOL sha_string(id value)
{
    if (![value isKindOfClass:NSString.class] || [value length] != 64) return NO;
    return [value rangeOfCharacterFromSet:[[NSCharacterSet characterSetWithCharactersInString:@"0123456789abcdef"] invertedSet]].location == NSNotFound;
}
/* Trust comes from the signed bundle, not this checksum or the archive's manifest. */
NSDictionary *HPExpectedDataManifest(NSDictionary *identity)
{
    if (![identity isKindOfClass:NSDictionary.class] || !integer(identity[@"schema"]) || [identity[@"schema"] intValue] != 1 ||
        !sha_string(identity[@"id"]) || ![identity[@"profile"] isKindOfClass:NSString.class] || ![identity[@"profile"] length]) return nil;
    NSMutableDictionary *files = [NSMutableDictionary dictionary];
    uint64_t total = 0;
    for (NSString *kind in @[@"stock_files", @"core_data"]) {
        NSDictionary *records = identity[kind];
        if (![records isKindOfClass:NSDictionary.class] || !records.count) return nil;
        for (NSString *name in records) {
            id row = records[name];
            if (!HPDataPathIsSafe(name) || ![row isKindOfClass:NSDictionary.class] || [row count] != 2 ||
                !integer(row[@"size"]) || [row[@"size"] longLongValue] < 0 || [row[@"size"] unsignedLongLongValue] > HP_MAX_FILE || !sha_string(row[@"sha256"])) return nil;
            total += [row[@"size"] unsignedLongLongValue];
            if (total > HP_MAX_TOTAL) return nil;
            files[[NSString stringWithFormat:@"%@/%@", [kind isEqual:@"stock_files"] ? @"game" : @"core-data", name]] = row;
        }
    }
    if (files.count >= 4096 || !HPDataPathsAreSafe(files.allKeys)) return nil;
    return @{@"schema":@1, @"format":@"halopad-data", @"profile":identity[@"profile"], @"core_id":identity[@"id"], @"files":files};
}

