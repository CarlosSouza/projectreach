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

/* argv and the environment, copied into guest memory */
static uint32_t make_boot(const char **environment, int count, int argc, char **argv)
{
	uint32_t boot = xg_map(0x10000, PROT_READ | PROT_WRITE);
	uint32_t *words = G(uint32_t *, boot);
	uint32_t argv_list = boot + 32, environment_list = argv_list + 4 * 16, strings = environment_list + 4 * 64;
	int index;
	for (index = 0; index < argc && index < 15; index++)
	{
		strcpy(G(char *, strings), argv[index]);
		G(uint32_t *, argv_list)[index] = strings;
		strings += (uint32_t)strlen(argv[index]) + 1;
	}
	G(uint32_t *, argv_list)[index] = 0;
	for (index = 0; index < count && index < 63; index++)
	{
		strcpy(G(char *, strings), environment[index]);
		G(uint32_t *, environment_list)[index] = strings;
		strings += (uint32_t)strlen(environment[index]) + 1;
	}
	G(uint32_t *, environment_list)[index] = 0;
	words[0] = (uint32_t)(argc < 15 ? argc : 15);
	words[1] = argv_list;
	words[2] = environment_list;
	words[3] = XG_PAGE;
	return boot;
}

int main(int argc, char **argv)
{
	const char *image_path = NULL, *angle = getenv("XG_ANGLE_DIR");
	char buffers[6][1100];
	const char *environment[8];
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
	if (getenv("HALO_DISPLAY_WIDTH"))
	{
		static char width[64];
		snprintf(width, sizeof(width), "HALO_DISPLAY_WIDTH=%s", getenv("HALO_DISPLAY_WIDTH"));
		environment[count++] = width;
	}
	boot = make_boot(environment, count, guest_argc, guest_argv);

	stack = xg_map(16u * 1024u * 1024u, PROT_READ | PROT_WRITE);
	if (!boot || !stack)
		xg_fatal("cannot allocate the game's stack");
	xg_log("data %s, saves %s", xg_paths.data_root, xg_paths.save_root);
	xg_call_on(G(uintptr_t, stack + 16u * 1024u * 1024u), xg_header->start, boot, 0, 0, 0);
	xg_fatal("the game returned from its entry point");
}
