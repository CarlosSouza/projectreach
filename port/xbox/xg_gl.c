/*
 * xg_gl.c: the guest's OpenGL ES helper imports (upstream's host_gl.c) and a
 * frame dump for testing, on any platform (xg_gl_proc resolves functions).
 */
#include "xg_host.h"

#include <GLES3/gl32.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void *xg_gl_proc(const char *name);

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
static void (*p_glReadPixels)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void *);

static void helpers_load(void)
{
	if (p_glGetString)
		return;
	p_glGetString = xg_gl_proc("glGetString");
	p_glGetStringi = xg_gl_proc("glGetStringi");
	p_glGetIntegerv = xg_gl_proc("glGetIntegerv");
	p_glBindBuffer = xg_gl_proc("glBindBuffer");
	p_glMapBufferRange = xg_gl_proc("glMapBufferRange");
	p_glUnmapBuffer = xg_gl_proc("glUnmapBuffer");
	p_glBufferSubData = xg_gl_proc("glBufferSubData");
	p_glFenceSync = xg_gl_proc("glFenceSync");
	p_glDeleteSync = xg_gl_proc("glDeleteSync");
	p_glClientWaitSync = xg_gl_proc("glClientWaitSync");
	p_glReadPixels = xg_gl_proc("glReadPixels");
}

static void copy_out(uint32_t buffer, uint32_t size, const char *text)
{
	if (!size)
		return;
	strncpy(G(char *, buffer), text ? text : "", size - 1);
	G(char *, buffer)[size - 1] = 0;
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

void xh_host_android_path(int which, uint32_t buffer, uint32_t size)
{
	copy_out(buffer, size, which ? xg_paths.save_root : xg_paths.data_root);
}

/* XG_FRAME_DUMP=<path>: every XG_FRAME_DUMP_SECONDS (default 5), the frame
 * about to be shown (the bound framebuffer) is written to <path> as a PPM
 * image, for testing without capturing anything else on screen */
void xg_gl_frame_dump(int width, int height)
{
	static const char *path;
	static int checked;
	static time_t next;
	unsigned char *pixels;
	FILE *file;
	int row;
	if (!checked)
	{
		checked = 1;
		path = getenv("XG_FRAME_DUMP");
	}
	if (!path || time(NULL) < next || width <= 0 || height <= 0)
		return;
	next = time(NULL) + (getenv("XG_FRAME_DUMP_SECONDS") ? atoi(getenv("XG_FRAME_DUMP_SECONDS")) : 5);
	helpers_load();
	pixels = malloc((size_t)width * height * 4);
	if (!pixels)
		return;
	p_glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	file = fopen(path, "wb");
	if (file)
	{
		fprintf(file, "P6\n%d %d\n255\n", width, height);
		/* OpenGL rows run bottom to top */
		for (row = height - 1; row >= 0; row--)
		{
			int column;
			for (column = 0; column < width; column++)
				fwrite(pixels + ((size_t)row * width + column) * 4, 1, 3, file);
		}
		fclose(file);
	}
	free(pixels);
}
