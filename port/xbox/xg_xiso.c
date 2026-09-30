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

#define SECTOR 2048ull

struct entry { char name[256]; uint32_t sector, size; int directory; };

static int read_at(FILE *file, uint64_t offset, void *buffer, size_t size)
{
	return fseeko(file, (off_t)offset, SEEK_SET) == 0 && fread(buffer, 1, size, file) == size;
}

static int list_directory(FILE *file, uint64_t base, uint32_t sector, uint32_t size, struct entry *entries, int capacity)
{
	uint8_t *data = malloc(size);
	int count = 0, top = 0;
	uint32_t stack[512];
	if (!data || !read_at(file, base + sector * SECTOR, data, size))
	{
		free(data);
		return -1;
	}
	stack[top++] = 0;
	while (top && count < capacity)
	{
		uint32_t offset = stack[--top];
		uint16_t left, right;
		if (offset + 14 > size)
			continue;
		left = (uint16_t)(data[offset] | data[offset + 1] << 8);
		right = (uint16_t)(data[offset + 2] | data[offset + 3] << 8);
		if (left == 0xffff)
			continue;
		memcpy(&entries[count].sector, data + offset + 4, 4);
		memcpy(&entries[count].size, data + offset + 8, 4);
		entries[count].directory = (data[offset + 12] & 0x10) != 0;
		{
			size_t length = data[offset + 13];
			if (offset + 14 + length > size)
				length = 0;
			memcpy(entries[count].name, data + offset + 14, length);
			entries[count].name[length] = 0;
		}
		count++;
		if (left && top < 512) stack[top++] = left * 4u;
		if (right && top < 512) stack[top++] = right * 4u;
	}
	free(data);
	return count;
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
	uint64_t base = 0, total = 0, done = 0;
	char magic[20], partial[1100], target[1100];
	uint32_t root_sector = 0, root_size = 0;
	int roots, count, index, found = 0, result = -1;
	const struct entry *maps_entry;
	if (build_size) *build = 0;
	if (!file || !root || !maps)
	{
		snprintf(error, error_size, "The disc image could not be opened.");
		goto done;
	}
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
	roots = list_directory(file, base, root_sector, root_size, root, 256);
	maps_entry = roots > 0 ? find(root, roots, "maps") : NULL;
	if (!maps_entry || !find(root, roots, "default.xbe"))
	{
		snprintf(error, error_size, "This disc image has no maps folder: it is not Halo: Combat Evolved.");
		goto done;
	}
	count = list_directory(file, base, maps_entry->sector, maps_entry->size, maps, 256);
	if (count <= 0 || !find(maps, count, "ui.map"))
	{
		snprintf(error, error_size, "This disc image has no ui.map: it is not Halo: Combat Evolved.");
		goto done;
	}
	for (index = 0; index < count; index++)
		if (!maps[index].directory)
			total += maps[index].size;
	snprintf(partial, sizeof(partial), "%s/maps.partial", destination);
	snprintf(target, sizeof(target), "%s/maps", destination);
	mkdir(destination, 0755);
	mkdir(partial, 0755);
	for (index = 0; index < count; index++)
	{
		char path[1400];
		FILE *out;
		uint64_t remaining = maps[index].size, offset = base + maps[index].sector * SECTOR;
		static uint8_t chunk[1 << 20];
		if (maps[index].directory)
			continue;
		if (build_size && !*build && !strcasecmp(maps[index].name, "ui.map"))
		{
			uint8_t header[0x60];
			if (read_at(file, offset, header, sizeof(header)) && !memcmp(header, "daeh", 4))
			{
				memcpy(build, header + 0x40, build_size - 1 < 32 ? build_size - 1 : 32);
				build[build_size - 1 < 32 ? build_size - 1 : 32] = 0;
			}
		}
		snprintf(path, sizeof(path), "%s/%s", partial, maps[index].name);
		out = fopen(path, "wb");
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
		fclose(out);
	}
	if (rename(partial, target))
	{
		snprintf(error, error_size, "Could not finish the maps folder.");
		goto done;
	}
	result = 0;
done:
	if (file) fclose(file);
	free(root);
	free(maps);
	return result;
}
