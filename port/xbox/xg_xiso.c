/*
 * xg_xiso.c: copies maps/ out of the player's Halo Xbox disc image, on the
 * device (the same reading as scripts/xbox/extract-maps.py; XDVDFS: 2048-byte
 * sectors, "MICROSOFT*XBOX*MEDIA" at 0x10000 in the game partition, and
 * directories stored as binary trees of entries).
 */
#include "xg_xiso.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define SECTOR 2048ull
#define MAX_DIRECTORY (1u << 20)
#define MAX_MAP_SIZE (512u << 20)
#define MAX_MAP_TOTAL (3ull << 30)

struct entry { char name[256]; uint32_t sector, size; int directory; };

static int read_at(FILE *file, uint64_t offset, void *buffer, size_t size)
{
	return fseeko(file, (off_t)offset, SEEK_SET) == 0 && fread(buffer, 1, size, file) == size;
}

static int in_image(uint64_t length, uint64_t offset, uint64_t size)
{
	return offset <= length && size <= length - offset;
}

static int safe_name(const char *name, size_t length)
{
	if (!length || (length == 1 && name[0] == '.') || (length == 2 && !memcmp(name, "..", 2))) return 0;
	for (size_t i = 0; i < length; i++)
		if ((unsigned char)name[i] < 32 || name[i] == 127 || name[i] == '/' || name[i] == '\\') return 0;
	return 1;
}

static int list_directory(FILE *file, uint64_t image_size, uint64_t base, uint32_t sector, uint32_t size, struct entry *entries, int capacity)
{
	if (size < 14 || size > MAX_DIRECTORY || !in_image(image_size, base + sector * SECTOR, size)) return -1;
	uint8_t *data = malloc(size), *seen = calloc((size + 3u) / 4u, 1);
	int count = 0, top = 0;
	uint32_t stack[512];
	if (!data || !seen || !read_at(file, base + sector * SECTOR, data, size))
	{
		free(data); free(seen);
		return -1;
	}
	stack[top++] = 0;
	while (top)
	{
		uint32_t offset = stack[--top];
		uint16_t left, right;
		if (offset + 14 > size || seen[offset / 4] || count == capacity) goto invalid;
		seen[offset / 4] = 1;
		left = (uint16_t)(data[offset] | data[offset + 1] << 8);
		right = (uint16_t)(data[offset + 2] | data[offset + 3] << 8);
		if (left == 0xffff)
			continue;
		memcpy(&entries[count].sector, data + offset + 4, 4);
		memcpy(&entries[count].size, data + offset + 8, 4);
		entries[count].directory = (data[offset + 12] & 0x10) != 0;
		{
			size_t length = data[offset + 13];
			if (offset + 14 + length > size || !safe_name((char *)data + offset + 14, length)) goto invalid;
			memcpy(entries[count].name, data + offset + 14, length);
			entries[count].name[length] = 0;
		}
		if (!in_image(image_size, base + entries[count].sector * SECTOR, entries[count].size)) goto invalid;
		for (int i = 0; i < count; i++)
			if (!strcasecmp(entries[i].name, entries[count].name)) goto invalid;
		count++;
		if (top + !!left + !!right > 512) goto invalid;
		if (left) stack[top++] = left * 4u;
		if (right) stack[top++] = right * 4u;
	}
	free(data); free(seen);
	return count;
invalid:
	free(data); free(seen);
	return -1;
}

static const struct entry *find(const struct entry *entries, int count, const char *name)
{
	int index;
	for (index = 0; index < count; index++)
		if (!strcasecmp(entries[index].name, name))
			return &entries[index];
	return NULL;
}

int xg_extract_maps(const char *image_path, const char *destination, xg_extract_progress progress, void *context,
	char *build, size_t build_size, char *error, size_t error_size)
{
	static const uint64_t partitions[] = { 0, 0x18300000ull, 0x2080000ull, 0xfd90000ull, 0x89d80000ull };
	FILE *file = fopen(image_path, "rb");
	struct entry *root = calloc(256, sizeof(*root)), *maps = calloc(256, sizeof(*maps));
	uint64_t base = 0, total = 0, done = 0, image_size = 0;
	char magic[20], partial[1100];
	char maps_build[33] = {0};
	int destination_fd = -1, partial_fd = -1;
	struct stat info;
	uint32_t root_sector = 0, root_size = 0;
	int roots, count, index, found = 0, result = -1;
	const struct entry *maps_entry;
	if (build_size) *build = 0;
	if (error_size) *error = 0;
	if (!file || !root || !maps)
	{
		snprintf(error, error_size, "The disc image could not be opened.");
		goto done;
	}
	if (fstat(fileno(file), &info) || !S_ISREG(info.st_mode) || info.st_size < 0)
	{
		snprintf(error, error_size, "The disc image must be a readable file.");
		goto done;
	}
	image_size = (uint64_t)info.st_size;
	for (index = 0; index < 5 && !found; index++)
		if (read_at(file, partitions[index] + 0x10000, magic, 20) && !memcmp(magic, "MICROSOFT*XBOX*MEDIA", 20))
		{
			base = partitions[index];
			found = 1;
		}
	if (!found || !read_at(file, base + 0x10000 + 20, &root_sector, 4) || !read_at(file, base + 0x10000 + 24, &root_size, 4))
	{
		snprintf(error, error_size, "This is not an Xbox disc image.");
		goto done;
	}
	roots = list_directory(file, image_size, base, root_sector, root_size, root, 256);
	maps_entry = roots > 0 ? find(root, roots, "maps") : NULL;
	const struct entry *xbe = roots > 0 ? find(root, roots, "default.xbe") : NULL;
	if (!maps_entry || !maps_entry->directory || !xbe || xbe->directory || xbe->size < 4 ||
		!read_at(file, base + xbe->sector * SECTOR, magic, 4) || memcmp(magic, "XBEH", 4))
	{
		snprintf(error, error_size, "This disc image has no maps folder: it is not Halo: Combat Evolved.");
		goto done;
	}
	count = list_directory(file, image_size, base, maps_entry->sector, maps_entry->size, maps, 256);
	if (count <= 0 || !find(maps, count, "ui.map"))
	{
		snprintf(error, error_size, "This disc image has no ui.map: it is not Halo: Combat Evolved.");
		goto done;
	}
	/* Validate the entire inventory before creating or writing any destination. */
	for (index = 0; index < count; index++)
	{
		uint8_t header[0x800];
		size_t length = strlen(maps[index].name);
		for (size_t i = 0; i < length; i++)
		{
			unsigned char c = (unsigned char)maps[index].name[i];
			if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-')) goto bad_maps;
			if (c >= 'A' && c <= 'Z') maps[index].name[i] = (char)(c + ('a' - 'A'));
		}
		if (maps[index].directory || length < 5 || strcmp(maps[index].name + length - 4, ".map") ||
			maps[index].size < sizeof(header) || maps[index].size > MAX_MAP_SIZE ||
			!read_at(file, base + maps[index].sector * SECTOR, header, sizeof(header)) ||
			memcmp(header, "daeh", 4) || memcmp(header + 4, "\5\0\0\0", 4) || memcmp(header + 0x7fc, "toof", 4) ||
			!header[0x40] || !memchr(header + 0x40, 0, 32)) goto bad_maps;
		if (!*maps_build) memcpy(maps_build, header + 0x40, 32);
		if (strncmp(maps_build, (char *)header + 0x40, 32)) goto bad_maps;
		total += maps[index].size;
		if (total > MAX_MAP_TOTAL) goto bad_maps;
	}
	int n = snprintf(partial, sizeof(partial), "%s/maps.import-XXXXXX", destination);
	if (n < 0 || n >= (int)sizeof(partial))
	{
		snprintf(error, error_size, "The destination path is too long.");
		goto done;
	}
	if (mkdir(destination, 0755) && errno != EEXIST) goto destination_error;
	destination_fd = open(destination, O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
	if (destination_fd < 0) goto destination_error;
	if (!fstatat(destination_fd, "maps", &info, AT_SYMLINK_NOFOLLOW) || errno != ENOENT)
	{
		snprintf(error, error_size, "Your existing maps were kept. Import into a new Halo Xbox folder.");
		goto done;
	}
	if (!mkdtemp(partial)) goto destination_error;
	const char *partial_name = strrchr(partial, '/') + 1;
	partial_fd = openat(destination_fd, partial_name, O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
	if (partial_fd < 0) goto destination_error;
	for (index = 0; index < count; index++)
	{
		FILE *out;
		uint64_t remaining = maps[index].size, offset = base + maps[index].sector * SECTOR;
		static uint8_t chunk[1 << 20];
		int fd = openat(partial_fd, maps[index].name, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0600);
		out = fd < 0 ? NULL : fdopen(fd, "wb");
		if (fd >= 0 && !out) close(fd);
		if (!out || fseeko(file, (off_t)offset, SEEK_SET))
		{
			if (out) fclose(out);
			snprintf(error, error_size, "Could not write %s (is there room for about 2 GB?).", maps[index].name);
			goto done;
		}
		while (remaining)
		{
			size_t want = remaining < sizeof(chunk) ? (size_t)remaining : sizeof(chunk);
			if (fread(chunk, 1, want, file) != want || fwrite(chunk, 1, want, out) != want)
			{
				fclose(out);
				snprintf(error, error_size, "Copying %s failed (is there room for about 2 GB?).", maps[index].name);
				goto done;
			}
			remaining -= want;
			done += want;
			if (progress)
				progress((double)done / (double)total, context);
		}
		if (fclose(out))
		{
			snprintf(error, error_size, "Finishing %s failed; your existing maps were kept.", maps[index].name);
			goto done;
		}
	}
	/* Exclusive publication also protects a maps folder created during copying. */
	if (renameatx_np(destination_fd, partial_name, destination_fd, "maps", RENAME_EXCL))
	{
		snprintf(error, error_size, "Could not finish the maps folder.");
		goto done;
	}
	if (build_size) snprintf(build, build_size, "%s", maps_build);
	result = 0;
	goto done;
bad_maps:
	snprintf(error, error_size, "The maps directory is invalid or contains incompatible maps. Choose your Halo Xbox disc image.");
	goto done;
destination_error:
	snprintf(error, error_size, "The maps folder could not be created safely (is there room for about 2 GB?).");
done:
	if (partial_fd >= 0) close(partial_fd);
	if (destination_fd >= 0) close(destination_fd);
	if (file) fclose(file);
	free(root);
	free(maps);
	return result;
}
