/*
 * xg_sdl.c: SDL3 and OpenGL ES services for the Xbox engine's guest
 * (the host half of upstream's port/android/guest/runtime/guest_sdl.c).
 *
 * SDL objects are pointers here, so the guest sees small integer handles.
 * SDL_Event has the same 128-byte layout on both sides for every event the
 * platform layer reads, so events are written straight into guest memory.
 */
#include "xg_host.h"

#include <GLES3/gl32.h>
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void xg_gl_load(void);

#define HANDLES 256
static void *handles[HANDLES];

static uint32_t handle_new(void *object)
{
	uint32_t index;
	if (!object)
		return 0;
	for (index = 1; index < HANDLES; index++)
		if (handles[index] == object)
			return index;
	for (index = 1; index < HANDLES; index++)
		if (!handles[index])
		{
			handles[index] = object;
			return index;
		}
	return 0;
}

static void *handle_get(uint32_t handle) { return handle < HANDLES ? handles[handle] : NULL; }

static void copy_out(uint32_t buffer, uint32_t size, const char *text)
{
	if (!size)
		return;
	strncpy(G(char *, buffer), text ? text : "", size - 1);
	G(char *, buffer)[size - 1] = 0;
}

int xh_host_sdl_init(uint32_t flags) { return SDL_Init(flags); }
int xh_host_sdl_set_hint(uint32_t name, uint32_t value) { return SDL_SetHint(G(const char *, name), GP(value)); }
void xh_host_sdl_get_error(uint32_t buffer, uint32_t size) { copy_out(buffer, size, SDL_GetError()); }
long long xh_host_sdl_ticks(void) { return (long long)SDL_GetTicks(); }
long long xh_host_sdl_thread_id(void) { return (long long)SDL_GetCurrentThreadID(); }

uint32_t xh_host_sdl_create_window(uint32_t title, int width, int height, long long flags)
{
	SDL_Window *window = SDL_CreateWindow(G(const char *, title), width, height, (SDL_WindowFlags)flags);
	int w = 0, h = 0;
	SDL_GetWindowSizeInPixels(window, &w, &h);
	xg_log("window %dx%d (flags 0x%llx): %dx%d pixels", width, height, flags, w, h);
	return handle_new(window);
}

void xh_host_sdl_window_size_in_pixels(uint32_t window, uint32_t width, uint32_t height)
{
	int w = 0, h = 0;
	SDL_GetWindowSizeInPixels(handle_get(window), &w, &h);
	if (width) *G(int *, width) = w;
	if (height) *G(int *, height) = h;
}

int xh_host_sdl_set_relative_mouse(uint32_t window, int enabled)
{
	return SDL_SetWindowRelativeMouseMode(handle_get(window), enabled != 0);
}

int xh_host_sdl_gl_set_attribute(int attribute, int value) { return SDL_GL_SetAttribute((SDL_GLAttr)attribute, value); }

uint32_t xh_host_sdl_gl_create_context(uint32_t window)
{
	SDL_GLContext context = SDL_GL_CreateContext(handle_get(window));
	if (!context)
	{
		xg_log("cannot create the OpenGL ES context: %s", SDL_GetError());
		return 0;
	}
	xg_gl_load();
	return handle_new(context);
}

int xh_host_sdl_gl_make_current(uint32_t window, uint32_t context)
{
	return SDL_GL_MakeCurrent(handle_get(window), handle_get(context));
}

int xh_host_sdl_gl_set_swap_interval(int interval) { return SDL_GL_SetSwapInterval(interval); }
/* XG_FRAME_DUMP=<path>: every XG_FRAME_DUMP_SECONDS (default 5), write the
 * frame about to be shown to <path> as a PPM image (testing without
 * capturing the rest of the screen) */
static void frame_dump(uint32_t window)
{
	static const char *path;
	static Uint64 next;
	static int checked;
	static void (*read_pixels)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void *);
	int width = 0, height = 0, row;
	unsigned char *pixels;
	FILE *file;
	if (!checked)
	{
		checked = 1;
		path = getenv("XG_FRAME_DUMP");
		read_pixels = (void *)SDL_GL_GetProcAddress("glReadPixels");
	}
	if (!path || !read_pixels || SDL_GetTicks() < next)
		return;
	next = SDL_GetTicks() + 1000u * (getenv("XG_FRAME_DUMP_SECONDS") ? (unsigned)atoi(getenv("XG_FRAME_DUMP_SECONDS")) : 5u);
	SDL_GetWindowSizeInPixels(handle_get(window), &width, &height);
	pixels = malloc((size_t)width * height * 4);
	if (!pixels)
		return;
	read_pixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	file = fopen(path, "wb");
	if (file)
	{
		fprintf(file, "P6\n%d %d\n255\n", width, height);
		for (row = 0; row < height; row++)
		{
			int column;
			for (column = 0; column < width; column++)
				fwrite(pixels + ((size_t)row * width + column) * 4, 1, 3, file);
		}
		fclose(file);
	}
	free(pixels);
}

int xh_host_sdl_gl_swap_window(uint32_t window)
{
	frame_dump(window);
	return SDL_GL_SwapWindow(handle_get(window));
}
int xh_host_sdl_poll_event(uint32_t event) { return SDL_PollEvent(G(SDL_Event *, event)); }
/* the game puts its internet invite link on the clipboard at start-up and
 * joins any link it finds there; HaloPad keeps the player's clipboard out of
 * it until invites have a proper place in the app */
int xh_host_sdl_set_clipboard_text(uint32_t text) { (void)text; return 1; }

void xh_host_sdl_get_clipboard_text(uint32_t buffer, uint32_t size)
{
	copy_out(buffer, size, "");
}

int xh_host_sdl_show_toast(uint32_t message, int duration, int gravity, int x, int y)
{
	(void)duration; (void)gravity; (void)x; (void)y;
	xg_log("%s", G(const char *, message));
	return 1;
}

int xh_host_sdl_show_simple_message_box(uint32_t flags, uint32_t title, uint32_t message)
{
	xg_log("message: %s: %s", G(const char *, title), G(const char *, message));
	return SDL_ShowSimpleMessageBox(flags, G(const char *, title), G(const char *, message), NULL);
}

int xh_host_sdl_get_gamepads(uint32_t ids, int capacity)
{
	int count = 0, index;
	SDL_JoystickID *list = SDL_GetGamepads(&count);
	for (index = 0; index < count && index < capacity; index++)
		G(uint32_t *, ids)[index] = list[index];
	SDL_free(list);
	return index;
}

uint32_t xh_host_sdl_open_gamepad(uint32_t id) { return handle_new(SDL_OpenGamepad(id)); }
uint32_t xh_host_sdl_gamepad_from_id(uint32_t id) { return handle_new(SDL_GetGamepadFromID(id)); }
int xh_host_sdl_gamepad_axis(uint32_t pad, int axis) { return SDL_GetGamepadAxis(handle_get(pad), (SDL_GamepadAxis)axis); }
int xh_host_sdl_gamepad_button(uint32_t pad, int button) { return SDL_GetGamepadButton(handle_get(pad), (SDL_GamepadButton)button); }
int xh_host_sdl_gamepad_type(uint32_t pad) { return SDL_GetGamepadType(handle_get(pad)); }

int xh_host_sdl_rumble_gamepad(uint32_t pad, uint32_t low, uint32_t high, uint32_t milliseconds)
{
	return SDL_RumbleGamepad(handle_get(pad), (Uint16)low, (Uint16)high, milliseconds);
}

/* ---------- audio: SDL's callback runs the guest's on SDL's audio thread,
 * switched to a guest stack by xg_enter; SDL's stream lock is recursive, so
 * the guest may put data into the stream from inside the callback */

struct audio { uint32_t callback, userdata, handle; };

static void SDLCALL audio_callback(void *context, SDL_AudioStream *stream, int additional, int total)
{
	struct audio *audio = context;
	(void)stream;
	xg_enter(audio->callback, audio->userdata, audio->handle, (uint32_t)additional, (uint32_t)total);
}

uint32_t xh_host_sdl_open_audio_stream(uint32_t device, uint32_t spec, uint32_t callback, uint32_t userdata)
{
	struct audio *audio = SDL_calloc(1, sizeof(*audio));
	SDL_AudioStream *stream;
	audio->callback = callback;
	audio->userdata = userdata;
	stream = SDL_OpenAudioDeviceStream(device, G(const SDL_AudioSpec *, spec), callback ? audio_callback : NULL, audio);
	if (!stream)
	{
		xg_log("cannot open audio: %s", SDL_GetError());
		return 0;
	}
	audio->handle = handle_new(stream);
	return audio->handle;
}

int xh_host_sdl_put_audio_stream_data(uint32_t stream, uint32_t data, int length)
{
	return SDL_PutAudioStreamData(handle_get(stream), G(const void *, data), length);
}

int xh_host_sdl_resume_audio_stream_device(uint32_t stream) { return SDL_ResumeAudioStreamDevice(handle_get(stream)); }

/* ---------- OpenGL ES helpers (upstream's host_gl.c) */

static const GLubyte *(*p_glGetString)(GLenum);
static const GLubyte *(*p_glGetStringi)(GLenum, GLuint);
static void (*p_glGetIntegerv)(GLenum, GLint *);
static void (*p_glBindBuffer)(GLenum, GLuint);
static void *(*p_glMapBufferRange)(GLenum, GLintptr, GLsizeiptr, GLbitfield);
static GLboolean (*p_glUnmapBuffer)(GLenum);
static void (*p_glBufferSubData)(GLenum, GLintptr, GLsizeiptr, const void *);
static GLsync (*p_glFenceSync)(GLenum, GLbitfield);
static void (*p_glDeleteSync)(GLsync);
static GLenum (*p_glClientWaitSync)(GLsync, GLbitfield, GLuint64);

static void helpers_load(void)
{
	if (p_glGetString)
		return;
	p_glGetString = (void *)SDL_GL_GetProcAddress("glGetString");
	p_glGetStringi = (void *)SDL_GL_GetProcAddress("glGetStringi");
	p_glGetIntegerv = (void *)SDL_GL_GetProcAddress("glGetIntegerv");
	p_glBindBuffer = (void *)SDL_GL_GetProcAddress("glBindBuffer");
	p_glMapBufferRange = (void *)SDL_GL_GetProcAddress("glMapBufferRange");
	p_glUnmapBuffer = (void *)SDL_GL_GetProcAddress("glUnmapBuffer");
	p_glBufferSubData = (void *)SDL_GL_GetProcAddress("glBufferSubData");
	p_glFenceSync = (void *)SDL_GL_GetProcAddress("glFenceSync");
	p_glDeleteSync = (void *)SDL_GL_GetProcAddress("glDeleteSync");
	p_glClientWaitSync = (void *)SDL_GL_GetProcAddress("glClientWaitSync");
}

void xh_host_gl_get_string(uint32_t name, int index, uint32_t buffer, uint32_t size)
{
	helpers_load();
	copy_out(buffer, size, (const char *)(index >= 0 ? p_glGetStringi(name, (GLuint)index) : p_glGetString(name)));
}

int xh_host_gl_has_extension(uint32_t name)
{
	GLint count = 0, index;
	helpers_load();
	p_glGetIntegerv(GL_NUM_EXTENSIONS, &count);
	for (index = 0; index < count; index++)
	{
		const char *extension = (const char *)p_glGetStringi(GL_EXTENSIONS, (GLuint)index);
		if (extension && !strcmp(extension, G(const char *, name)))
			return 1;
	}
	return 0;
}

uint32_t xh_host_gl_read_buffer_word(uint32_t buffer, uint32_t offset)
{
	uint32_t value = 0;
	GLint previous = 0;
	const void *mapping;
	helpers_load();
	p_glGetIntegerv(GL_ATOMIC_COUNTER_BUFFER_BINDING, &previous);
	p_glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, buffer);
	mapping = p_glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, offset, sizeof(value), GL_MAP_READ_BIT);
	if (mapping)
	{
		memcpy(&value, mapping, sizeof(value));
		p_glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
	}
	p_glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, (GLuint)previous);
	return value;
}

void xh_host_gl_buffer_write(uint32_t target, uint32_t offset, uint32_t size, uint32_t data)
{
	void *mapping;
	helpers_load();
	mapping = p_glMapBufferRange(target, offset, size, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
	if (!mapping)
	{
		p_glBufferSubData(target, offset, size, G(const void *, data));
		return;
	}
	memcpy(mapping, G(const void *, data), size);
	p_glUnmapBuffer(target);
}

#define FENCES 8
static GLsync fences[FENCES];

void xh_host_gl_fence_frame(uint32_t slot)
{
	helpers_load();
	if (slot >= FENCES)
		return;
	if (fences[slot])
		p_glDeleteSync(fences[slot]);
	fences[slot] = p_glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

void xh_host_gl_wait_frame(uint32_t slot)
{
	helpers_load();
	if (slot >= FENCES || !fences[slot])
		return;
	p_glClientWaitSync(fences[slot], GL_SYNC_FLUSH_COMMANDS_BIT, 1000000000ull);
	p_glDeleteSync(fences[slot]);
	fences[slot] = NULL;
}

/* ---------- paths (upstream asks for Android's storage directories) */

void xh_host_android_path(int which, uint32_t buffer, uint32_t size)
{
	copy_out(buffer, size, which ? xg_paths.save_root : xg_paths.data_root);
}
