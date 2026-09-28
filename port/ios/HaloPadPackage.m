/* Prepared-data ZIP reader. ZIP structure follows PKWARE APPNOTE 6.3.10:
   https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT
   Stored/DEFLATE, ZIP64 and data descriptors; no executable loading. zlib is an SDK library.
   Both metadata and streaming output are bounded independently of archive claims. */
#import "HaloPadImport.h"
#import "HaloPadDataIdentity.h"
#import <CommonCrypto/CommonDigest.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>

#define HP_MAX_FILE (UINT64_C(2) << 30)
#define HP_MAX_TOTAL (UINT64_C(8) << 30)
#define HP_MAX_MANIFEST (2 * 1024 * 1024)
static BOOL zp_fail(NSError **error, NSString *message)
{
    if (error) *error = [NSError errorWithDomain:@"HaloPadPackage" code:1 userInfo:@{NSLocalizedDescriptionKey:message}];
    return NO;
}
static uint16_t u16(const uint8_t *p) { return p[0] | ((uint16_t)p[1] << 8); }
static uint32_t u32(const uint8_t *p) { return u16(p) | ((uint32_t)u16(p+2) << 16); }
static uint64_t u64(const uint8_t *p) { return u32(p) | ((uint64_t)u32(p+4) << 32); }
static BOOL read_at(int fd, uint64_t size, uint64_t at, void *buffer, size_t n)
{
    if (at > size || n > size - at) return NO;
    uint8_t *out = buffer;
    while (n) {
        ssize_t got = pread(fd, out, n, (off_t)at);
        if (got < 0 && errno == EINTR) continue;
        if (got <= 0) return NO;
        out += got; at += got; n -= got;
    }
    return YES;
}
static BOOL write_all(int fd, const void *bytes, size_t length)
{
    const uint8_t *p = bytes;
    while (length) {
        ssize_t n = write(fd, p, length);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return NO;
        p += n; length -= n;
    }
    return YES;
}
static BOOL integer(id n)
{
    return [n isKindOfClass:NSNumber.class] && CFGetTypeID((__bridge CFTypeRef)n) != CFBooleanGetTypeID() &&
           strchr("cCsSiIlLqQ", [(NSNumber *)n objCType][0]) != NULL;
}
/* Foundation checks JSON syntax. This second bounded walk rejects duplicate decoded keys,
   including escaped aliases, instead of accepting Foundation's last-key-wins behavior. */
static void spaces(NSString *s, NSUInteger *i)
{
    while (*i < s.length && [[NSCharacterSet whitespaceAndNewlineCharacterSet] characterIsMember:[s characterAtIndex:*i]]) (*i)++;
}
static NSString *json_string(NSString *s, NSUInteger *i)
{
    NSUInteger start = (*i)++;
    while (*i < s.length) {
        unichar c = [s characterAtIndex:(*i)++];
        if (c == '\\') { if (*i < s.length) (*i)++; else return nil; }
        else if (c == '"') {
            NSData *raw = [[s substringWithRange:NSMakeRange(start, *i - start)] dataUsingEncoding:NSUTF8StringEncoding];
            id value = [NSJSONSerialization JSONObjectWithData:raw options:NSJSONReadingFragmentsAllowed error:nil];
            return [value isKindOfClass:NSString.class] ? value : nil;
        }
    }
    return nil;
}
static BOOL json_unique(NSString *s, NSUInteger *i, unsigned depth)
{
    spaces(s, i);
    if (*i >= s.length || depth > 32) return NO;
    unichar c = [s characterAtIndex:*i];
    if (c == '"') return json_string(s, i) != nil;
    if (c != '{' && c != '[') {
        NSUInteger start = *i;
        while (*i < s.length && !strchr(",]} \t\r\n", [s characterAtIndex:*i])) (*i)++;
        return *i > start;
    }
    (*i)++; spaces(s, i);
    unichar close = c == '{' ? '}' : ']';
    NSMutableSet *keys = [NSMutableSet set];
    if (*i < s.length && [s characterAtIndex:*i] == close) { (*i)++; return YES; }
    while (*i < s.length) {
        if (c == '{') {
            spaces(s, i);
            if (*i >= s.length || [s characterAtIndex:*i] != '"') return NO;
            NSString *key = json_string(s, i);
            if (!key || [keys containsObject:key]) return NO;
            [keys addObject:key]; spaces(s, i);
            if (*i >= s.length || [s characterAtIndex:(*i)++] != ':') return NO;
        }
        if (!json_unique(s, i, depth + 1)) return NO;
        spaces(s, i);
        if (*i >= s.length) return NO;
        unichar next = [s characterAtIndex:(*i)++];
        if (next == close) return YES;
        if (next != ',') return NO;
    }
    return NO;
}
static BOOL same_json(id value, id expected)
{
    if ([expected isKindOfClass:NSDictionary.class]) {
        if (![value isKindOfClass:NSDictionary.class] || [value count] != [expected count]) return NO;
        for (NSString *key in expected) if (!same_json(value[key], expected[key])) return NO;
        return YES;
    }
    if ([expected isKindOfClass:NSNumber.class]) return integer(value) && [value isEqual:expected];
    return [value isKindOfClass:NSString.class] && [value isEqual:expected];
}

@interface HPZipEntry : NSObject
@property NSString *name;
@property uint64_t compressed, expanded, offset, dataOffset, end;
@property uint32_t crc;
@property uint16_t method, flags;
@end
@implementation HPZipEntry
@end

/* Resolve mandatory ZIP64 fields in their specified order. Unknown extra fields are inert. */
static BOOL zip64(const uint8_t *p, size_t n, uint64_t *expanded, uint64_t *compressed, uint64_t *offset, uint32_t disk)
{
    BOOL found = NO;
    for (size_t i = 0; i < n;) {
        if (n - i < 4) return NO;
        uint16_t tag = u16(p+i), length = u16(p+i+2); i += 4;
        if (length > n - i) return NO;
        if (tag == 1) {
            if (found) return NO;
            found = YES;
            size_t at = 0;
            uint64_t *fields[] = {expanded, compressed, offset};
            for (int j = 0; j < 3; j++) if (fields[j] && *fields[j] == UINT32_MAX) {
                if (length - at < 8) return NO;
                *fields[j] = u64(p+i+at); at += 8;
            }
            if (disk == UINT16_MAX) {
                if (length - at < 4 || u32(p+i+at) != 0) return NO;
                disk = 0;
            }
        }
        i += length;
    }
    return disk == 0 && *expanded != UINT32_MAX && *compressed != UINT32_MAX && (!offset || *offset != UINT32_MAX);
}

@interface HPZipReader : NSObject
@property int fd;
@property uint64_t length;
@property NSMutableDictionary<NSString *, HPZipEntry *> *entries;
- (BOOL)index:(NSDictionary *)expected error:(NSError **)error;
- (BOOL)read:(HPZipEntry *)entry output:(int)output capture:(NSMutableData *)capture hash:(NSString *)hash error:(NSError **)error;
@end
@implementation HPZipReader
- (instancetype)init { if ((self = [super init])) _fd = -1; return self; }
- (void)dealloc { if (_fd >= 0) close(_fd); }
- (BOOL)index:(NSDictionary *)expected error:(NSError **)error
{
    uint8_t tail[65557];
    size_t n = (size_t)MIN(self.length, sizeof tail);
    if (n < 22 || !read_at(self.fd, self.length, self.length-n, tail, n)) return zp_fail(error, @"The package is truncated or unreadable.");
    NSInteger end = (NSInteger)n - 22;
    for (; end >= 0; end--)
        if (u32(tail+end) == 0x06054b50 && (size_t)end+22+u16(tail+end+20) == n) break;
    if (end < 0) return zp_fail(error, @"The ZIP directory is missing or damaged.");
    const uint8_t *e = tail+end;
    uint64_t count = u16(e+10), cdSize = u32(e+12), cd = u32(e+16), boundary = self.length-n+end;
    if (u16(e+4) || u16(e+6) || u16(e+8) != count) return zp_fail(error, @"Split ZIP packages are unsupported.");
    uint8_t locator[20];
    BOOL has64 = boundary >= 20 && read_at(self.fd, self.length, boundary-20, locator, 20) && u32(locator) == 0x07064b50;
    if (has64) {
        uint8_t record[56]; uint64_t pos = u64(locator+8);
        if (u32(locator+4) || u32(locator+16) != 1 || pos > boundary-20 || boundary-20-pos < 56 ||
            !read_at(self.fd, self.length, pos, record, 56) || u32(record) != 0x06064b50 ||
            u64(record+4) != boundary-20-pos-12 || u32(record+16) || u32(record+20) || u64(record+24) != u64(record+32) ||
            (count != UINT16_MAX && count != u64(record+32)) || (cdSize != UINT32_MAX && cdSize != u64(record+40)) || (cd != UINT32_MAX && cd != u64(record+48)))
            return zp_fail(error, @"The ZIP64 directory is invalid.");
        count = u64(record+32); cdSize = u64(record+40); cd = u64(record+48); boundary = pos;
    } else if (count == UINT16_MAX || cdSize == UINT32_MAX || cd == UINT32_MAX) return zp_fail(error, @"The ZIP64 directory is missing.");
    if (count != [expected count]+1 || count > 4096 || cd > boundary || cdSize != boundary-cd)
        return zp_fail(error, @"The package has an invalid directory or unexpected files.");
    self.entries = [NSMutableDictionary dictionary];
    uint64_t cursor = cd;
    for (uint64_t i = 0; i < count; i++) {
        uint8_t h[46];
        if (!read_at(self.fd, boundary, cursor, h, 46) || u32(h) != 0x02014b50) return zp_fail(error, @"The ZIP directory is damaged.");
        uint16_t nameLen = u16(h+28), extraLen = u16(h+30), commentLen = u16(h+32);
        uint64_t extent = 46 + nameLen + extraLen + commentLen;
        if (!nameLen || nameLen > 512 || extent > boundary-cursor) return zp_fail(error, @"Invalid ZIP filename or metadata length.");
        NSMutableData *variable = [NSMutableData dataWithLength:nameLen+extraLen];
        if (!read_at(self.fd, boundary, cursor+46, variable.mutableBytes, variable.length)) return zp_fail(error, @"Cannot read the ZIP directory.");
        HPZipEntry *entry = [HPZipEntry new];
        entry.name = [[NSString alloc] initWithBytes:variable.bytes length:nameLen encoding:NSUTF8StringEncoding];
        uint64_t expanded = u32(h+24), compressed = u32(h+20), offset = u32(h+42);
        if (!zip64((const uint8_t *)variable.bytes+nameLen, extraLen, &expanded, &compressed, &offset, u16(h+34))) return zp_fail(error, @"Invalid ZIP64 file metadata.");
        uint32_t attributes = u32(h+38), type = (attributes >> 16) & S_IFMT;
        entry.flags = u16(h+8); entry.method = u16(h+10); entry.crc = u32(h+16);
        entry.expanded = expanded; entry.compressed = compressed; entry.offset = offset;
        if (!HPDataPathIsSafe(entry.name) || self.entries[entry.name] || (type && type != S_IFREG) || (attributes & 0x18) ||
            (entry.flags & ~0x080e) || (entry.method != 0 && entry.method != 8)) return zp_fail(error, @"The package contains an unsafe path, link, or unsupported ZIP entry.");
        BOOL manifest = [entry.name isEqual:@"manifest.json"];
        NSDictionary *wanted = expected[entry.name];
        if ((!manifest && !wanted) || expanded > (manifest ? HP_MAX_MANIFEST : [wanted[@"size"] unsignedLongLongValue]) ||
            (!manifest && expanded != [wanted[@"size"] unsignedLongLongValue]) || compressed > HP_MAX_FILE + (8 << 20) ||
            (entry.method == 0 && compressed != expanded)) return zp_fail(error, @"The package file inventory or sizes do not match this app.");
        self.entries[entry.name] = entry; cursor += extent;
    }
    if (cursor != boundary || !self.entries[@"manifest.json"] || !HPDataPathsAreSafe(self.entries.allKeys)) return zp_fail(error, @"The package has ambiguous or missing files.");
    /* Check every local record, and reject overlaps, hidden records and trailing payloads. */
    NSArray<HPZipEntry *> *ordered = [self.entries.allValues sortedArrayUsingComparator:^NSComparisonResult(HPZipEntry *a, HPZipEntry *b) {
        return a.offset < b.offset ? NSOrderedAscending : a.offset > b.offset ? NSOrderedDescending : NSOrderedSame;
    }];
    cursor = 0;
    for (NSUInteger index = 0; index < ordered.count; index++) {
        HPZipEntry *entry = ordered[index];
        uint64_t nextOffset = index + 1 < ordered.count ? ordered[index+1].offset : cd;
        uint8_t h[30];
        if (entry.offset != cursor || !read_at(self.fd, cd, cursor, h, 30) || u32(h) != 0x04034b50 || u16(h+6) != entry.flags || u16(h+8) != entry.method)
            return zp_fail(error, @"The ZIP local records overlap or disagree with the directory.");
        uint16_t nameLen = u16(h+26), extraLen = u16(h+28);
        if (!nameLen || nameLen > 512) return zp_fail(error, @"Invalid local ZIP filename.");
        NSMutableData *variable = [NSMutableData dataWithLength:nameLen+extraLen];
        if (!read_at(self.fd, cd, cursor+30, variable.mutableBytes, variable.length)) return zp_fail(error, @"The local ZIP record is truncated.");
        NSData *rawName = [entry.name dataUsingEncoding:NSUTF8StringEncoding];
        if (rawName.length != nameLen || memcmp(rawName.bytes, variable.bytes, nameLen)) return zp_fail(error, @"The local ZIP filename disagrees with its directory.");
        uint64_t expanded = u32(h+22), compressed = u32(h+18);
        BOOL local64 = expanded == UINT32_MAX || compressed == UINT32_MAX;
        if (!zip64((const uint8_t *)variable.bytes+nameLen, extraLen, &expanded, &compressed, NULL, 0)) return zp_fail(error, @"Invalid local ZIP64 sizes.");
        if (!(entry.flags & 8) && (expanded != entry.expanded || compressed != entry.compressed || u32(h+14) != entry.crc))
            return zp_fail(error, @"The local ZIP sizes or checksum disagree with the directory.");
        entry.dataOffset = cursor + 30 + nameLen + extraLen;
        if (entry.dataOffset > cd || entry.compressed > cd-entry.dataOffset) return zp_fail(error, @"ZIP payload exceeds the file boundary.");
        cursor = entry.dataOffset + entry.compressed;
        if (entry.flags & 8) {
            uint8_t descriptor[24];
            if (!read_at(self.fd, cd, cursor, descriptor, 4)) return zp_fail(error, @"ZIP data descriptor is missing.");
            /* The optional signature can equal an unsigned descriptor's CRC. Try both
               interpretations, requiring the next physical record boundary as well. */
            BOOL matched = NO;
            for (size_t signature = 0; signature <= 4; signature += 4) {
                size_t length = signature + (local64 ? 20 : 12);
                if (cursor > nextOffset || length != nextOffset-cursor ||
                    !read_at(self.fd, cd, cursor, descriptor, length) || (signature && u32(descriptor) != 0x08074b50)) continue;
                const uint8_t *d = descriptor + signature;
                if (u32(d) == entry.crc && (local64 ? u64(d+4) : u32(d+4)) == entry.compressed && (local64 ? u64(d+12) : u32(d+8)) == entry.expanded) {
                    cursor += length; matched = YES; break;
                }
            }
            if (!matched) return zp_fail(error, @"ZIP data descriptor does not match the file.");
        }
        entry.end = cursor;
    }
    return cursor == cd ? YES : zp_fail(error, @"Unexpected bytes before the ZIP directory.");
}
- (BOOL)read:(HPZipEntry *)entry output:(int)output capture:(NSMutableData *)capture hash:(NSString *)hash error:(NSError **)error
{
    uint8_t in[65536], out[65536];
    z_stream stream = {0};
    BOOL deflated = entry.method == 8;
    if (deflated && inflateInit2(&stream, -MAX_WBITS) != Z_OK) return zp_fail(error, @"Could not initialize package decompression.");
    CC_SHA256_CTX context; CC_SHA256_Init(&context);
    uint64_t loaded = 0, written = 0; uLong crc = crc32(0, NULL, 0);
    BOOL ended = !deflated;
    @try {
        do {
            if (!stream.avail_in && loaded < entry.compressed) {
                size_t n = (size_t)MIN(sizeof in, entry.compressed-loaded);
                if (!read_at(self.fd, self.length, entry.dataOffset+loaded, in, n)) return zp_fail(error, @"The package changed or could not be read.");
                loaded += n; stream.next_in = in; stream.avail_in = (uInt)n;
            }
            const uint8_t *bytes = in;
            size_t made = stream.avail_in;
            if (deflated) {
                stream.next_out = out; stream.avail_out = sizeof out;
                uInt before = stream.avail_in;
                int rc = inflate(&stream, Z_NO_FLUSH);
                made = sizeof out-stream.avail_out; bytes = out;
                ended = rc == Z_STREAM_END;
                if ((rc != Z_OK && !ended) || (!ended && !made && before == stream.avail_in)) return zp_fail(error, @"The compressed package data is damaged.");
            } else stream.avail_in = 0;
            if (made > entry.expanded-written) return zp_fail(error, @"Expanded package data exceeds its verified size.");
            if (output >= 0 && !write_all(output, bytes, made)) return zp_fail(error, @"Could not save the imported data. Check available storage.");
            if (capture) [capture appendBytes:bytes length:made];
            CC_SHA256_Update(&context, bytes, (CC_LONG)made); crc = crc32(crc, bytes, (uInt)made); written += made;
            if (deflated && ended) break;
        } while (loaded < entry.compressed || stream.avail_in || (deflated && !ended));
        unsigned char digest[CC_SHA256_DIGEST_LENGTH]; CC_SHA256_Final(digest, &context);
        NSMutableString *actual = [NSMutableString string];
        for (int i = 0; i < CC_SHA256_DIGEST_LENGTH; i++) [actual appendFormat:@"%02x", digest[i]];
        if (!ended || loaded != entry.compressed || stream.avail_in || written != entry.expanded || crc != entry.crc || (hash && ![hash isEqual:actual]))
            return zp_fail(error, [NSString stringWithFormat:@"Package content verification failed: %@.", entry.name]);
        return YES;
    } @finally { if (deflated) inflateEnd(&stream); }
}
@end

BOOL HPImportGamePackage(NSURL *archive, NSURL *destination, NSDictionary *identity, NSURL **previous, NSError **error)
{
    if (previous) *previous = nil;
    if (error) *error = nil;
    NSDictionary *manifest = HPExpectedDataManifest(identity);
    if (!manifest) return zp_fail(error, @"The app's prepared-data identity is missing or invalid. Rebuild the app before preparing data.");
    if (!archive.isFileURL || !destination.isFileURL) return zp_fail(error, @"Choose a local prepared-data package.");
    HPZipReader *reader = [HPZipReader new];
    reader.fd = open(archive.path.fileSystemRepresentation, O_RDONLY | O_NOFOLLOW | O_NONBLOCK);
    struct stat info;
    if (reader.fd < 0 || fstat(reader.fd, &info) || !S_ISREG(info.st_mode) || info.st_size < 0 || (uint64_t)info.st_size > HP_MAX_TOTAL + (UINT64_C(1) << 30))
        return zp_fail(error, @"The package must be a readable regular ZIP file within the size limit.");
    reader.length = (uint64_t)info.st_size;
    if (![reader index:manifest[@"files"] error:error]) return NO;
    NSMutableData *raw = [NSMutableData data];
    if (![reader read:reader.entries[@"manifest.json"] output:-1 capture:raw hash:nil error:error]) return NO;
    NSString *json = [[NSString alloc] initWithData:raw encoding:NSUTF8StringEncoding];
    NSUInteger at = 0;
    id declared = [NSJSONSerialization JSONObjectWithData:raw options:0 error:nil];
    if (!json || !json_unique(json, &at, 0) || !same_json(declared, manifest)) return zp_fail(error, @"This package does not match the app's compiled core and stock files. Prepare it using this app build.");
    spaces(json, &at);
    if (at != json.length) return zp_fail(error, @"Unexpected content after the package manifest.");
    NSString *dst = destination.URLByStandardizingPath.path;
    NSString *parent = dst.stringByDeletingLastPathComponent;
    char *template = strdup([[parent stringByAppendingPathComponent:@".halopad-import-XXXXXX"] fileSystemRepresentation]);
    char *created = mkdtemp(template);
    NSString *stage = created ? [NSFileManager.defaultManager stringWithFileSystemRepresentation:created length:strlen(created)] : nil;
    free(template);
    if (!stage) return zp_fail(error, @"Could not create import staging. Check available storage.");
    BOOL published = NO;
    @try {
        for (NSString *name in [manifest[@"files"] allKeys]) {
            NSDictionary *record = manifest[@"files"][name];
            int output = -1;
            if ([name hasPrefix:@"game/"]) {
                NSString *path = [stage stringByAppendingPathComponent:[name substringFromIndex:5]];
                NSError *directoryError = nil;
                if (![NSFileManager.defaultManager createDirectoryAtPath:path.stringByDeletingLastPathComponent withIntermediateDirectories:YES attributes:nil error:&directoryError])
                    return zp_fail(error, @"Could not create the game folders. Check available storage.");
                output = open(path.fileSystemRepresentation, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0600);
                if (output < 0) return zp_fail(error, @"Could not create an imported file. Check available storage.");
            }
            BOOL ok = [reader read:reader.entries[name] output:output capture:nil hash:record[@"sha256"] error:error];
            if (output >= 0) {
                if (ok && fsync(output)) ok = zp_fail(error, @"Could not finish saving the imported data.");
                if (close(output) && ok) ok = zp_fail(error, @"Could not close the imported data file.");
            }
            if (!ok) return NO;
        }
        /* Core-data entries are verified then discarded: the app continues using signed
           bundled inert images, and never installs CPU code or imported registry state. */
        published = HPPublishGameImport(stage, dst, previous, error);
        return published;
    } @finally {
        if (!published) [NSFileManager.defaultManager removeItemAtPath:stage error:nil];
    }
}
