/* xg_xiso.h: maps/ from a Halo Xbox disc image (xg_xiso.c). */
#ifndef XG_XISO_H
#define XG_XISO_H
#include <stddef.h>

typedef void (*xg_extract_progress)(double fraction, void *context);

/* Validates a bounded map inventory before copying into a unique staging
 * directory. Exclusive publication never replaces existing maps or saves.
 * Incomplete stages are retained, never reused. build receives the common map
 * build only on success. 0 on success, else -1 with a player-facing error. */
int xg_extract_maps(const char *image_path, const char *destination, xg_extract_progress progress, void *context,
	char *build, size_t build_size, char *error, size_t error_size);

#endif
