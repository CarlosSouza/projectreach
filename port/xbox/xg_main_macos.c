/*
 * xg_main_macos.c: the Xbox engine as a plain macOS program (the Mac proof;
 * docs/XBOX-ENGINE.md). The game runs on the process's main thread, which
 * Cocoa needs for its window, on a stack switched into guest memory.
 *
 * usage: halopad-xbox --image halo_guest.elf --data <folder with maps/>
 *                     [--save <folder>] [--angle <folder with libEGL.dylib>]
 */
#include "xg_host.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>

extern char **environ;

struct xg_paths xg_paths;

static void *read_file(const char *path, size_t *size)
{
	FILE *file = fopen(path, "rb");
	void *data;
	long length;
	if (!file)
		return NULL;
	fseek(file, 0, SEEK_END);
	length = ftell(file);
	fseek(file, 0, SEEK_SET);
	data = malloc((size_t)length);
	if (data && fread(data, 1, (size_t)length, file) != (size_t)length)
	{
		free(data);
		data = NULL;
	}
	fclose(file);
	*size = (size_t)length;
	return data;
}

int main(int argc, char **argv)
{
	const char *image_path = NULL, *angle = getenv("XG_ANGLE_DIR");
	char buffers[6][1100];
	const char *environment[48];
	int count = 0, index, guest_argc = 1;
	char *guest_argv[16] = { "halo" };
	size_t size;
	void *image;
	uint32_t boot, stack;
	for (index = 1; index < argc; index++)
	{
		if (!strcmp(argv[index], "--image") && index + 1 < argc) image_path = argv[++index];
		else if (!strcmp(argv[index], "--data") && index + 1 < argc) strlcpy(xg_paths.data_root, argv[++index], sizeof(xg_paths.data_root));
		else if (!strcmp(argv[index], "--save") && index + 1 < argc) strlcpy(xg_paths.save_root, argv[++index], sizeof(xg_paths.save_root));
		else if (!strcmp(argv[index], "--angle") && index + 1 < argc) angle = argv[++index];
		else if (guest_argc < 15) guest_argv[guest_argc++] = argv[index];
	}
	if (!image_path || !*xg_paths.data_root)
	{
		fprintf(stderr, "usage: %s --image halo_guest.elf --data <folder with maps/> [--save dir] [--angle dir] [game arguments]\n", argv[0]);
		return 2;
	}
	if (!*xg_paths.save_root)
		snprintf(xg_paths.save_root, sizeof(xg_paths.save_root), "%s/save", xg_paths.data_root);
	mkdir(xg_paths.save_root, 0755);

	if (angle)
	{
		/* OpenGL ES through ANGLE (Metal) */
		snprintf(buffers[0], sizeof(buffers[0]), "%s/libEGL.dylib", angle);
		snprintf(buffers[1], sizeof(buffers[1]), "%s/libGLESv2.dylib", angle);
		SDL_SetHint(SDL_HINT_EGL_LIBRARY, buffers[0]);
		SDL_SetHint(SDL_HINT_OPENGL_LIBRARY, buffers[1]);
		SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1");
		/* ANGLE's Metal back end (its default here is Apple's OpenGL) */
		setenv("ANGLE_DEFAULT_PLATFORM", "metal", 0);
	}

	xg_install_signal_handlers();
	if (xg_memory_initialize())
		xg_fatal("cannot reserve guest memory");
	image = read_file(image_path, &size);
	if (!image || xg_load_image(image, size))
		xg_fatal("cannot load the game image %s", image_path);
	free(image);

	snprintf(buffers[2], sizeof(buffers[2]), "HOME=%s", xg_paths.save_root);
	snprintf(buffers[3], sizeof(buffers[3]), "HALO_DATA_ROOT=%s", xg_paths.data_root);
	snprintf(buffers[4], sizeof(buffers[4]), "HALO_SAVE_ROOT=%s", xg_paths.save_root);
	{
		time_t now = time(NULL);
		struct tm local;
		long offset;
		localtime_r(&now, &local);
		offset = -local.tm_gmtoff;
		snprintf(buffers[5], sizeof(buffers[5]), "TZ=<L>%s%ld:%02ld", offset < 0 ? "-" : "", labs(offset) / 3600, (labs(offset) / 60) % 60);
	}
	environment[count++] = buffers[2];
	environment[count++] = buffers[3];
	environment[count++] = buffers[4];
	environment[count++] = buffers[5];
	/* upstream's settings can also be given as HALO_* variables
	 * (port/linux/src/port_config.c); pass those through */
	{
		char **entry;
		for (entry = environ; *entry && count < 47; entry++)
			if (!strncmp(*entry, "HALO_", 5) && strncmp(*entry, "HALO_DATA_ROOT=", 15) && strncmp(*entry, "HALO_SAVE_ROOT=", 15))
				environment[count++] = *entry;
	}
	boot = xg_make_boot(environment, count, guest_argc, guest_argv);

	stack = xg_map(16u * 1024u * 1024u, PROT_READ | PROT_WRITE);
	if (!boot || !stack)
		xg_fatal("cannot allocate the game's stack");
	xg_log("data %s, saves %s", xg_paths.data_root, xg_paths.save_root);
	xg_call_on(G(uintptr_t, stack + 16u * 1024u * 1024u), xg_header->start, boot, 0, 0, 0);
	xg_fatal("the game returned from its entry point");
}
