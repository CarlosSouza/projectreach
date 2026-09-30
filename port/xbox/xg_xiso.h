/* xg_xiso.h: maps/ from a Halo Xbox disc image (xg_xiso.c). */
#ifndef XG_XISO_H
#define XG_XISO_H
#include <stddef.h>

typedef void (*xg_extract_progress)(double fraction, void *context);

/* copies the image's maps/ to <destination>/maps (through maps.partial, so an
 * interrupted copy never looks complete); build receives the maps' build
 * string. 0 on success, else -1 with a message for the player in error. */
int xg_extract_maps(const char *image_path, const char *destination, xg_extract_progress progress, void *context,
	char *build, size_t build_size, char *error, size_t error_size);

#endif
