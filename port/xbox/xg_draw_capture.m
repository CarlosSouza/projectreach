/* Opt-in Simulator diagnostics. Captured game-derived data stays in ref/.
 * Observe existing draws without relinking or modifying the guest's GL state. */
#import <Foundation/Foundation.h>
#include <TargetConditionals.h>
#if TARGET_OS_SIMULATOR
#import <OpenGLES/ES3/gl.h>
#include <stdlib.h>
#include <string.h>
#include "xg_host.h"

static NSData *shader_source(GLuint shader)
{
	GLint length = 0;
	glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &length);
	if (length <= 0 || length > 1024 * 1024) return nil;
	NSMutableData *data = [NSMutableData dataWithLength:length];
	GLsizei used = 0;
	glGetShaderSource(shader, length, &used, data.mutableBytes);
	data.length = used;
	return data;
}

static NSArray *float_bits(const GLfloat *values, int count)
{
	NSMutableArray *result = [NSMutableArray array];
	for (int i = 0; i < count; i++) {
		uint32_t bits;
		memcpy(&bits, &values[i], sizeof(bits));
		[result addObject:@(bits)];
	}
	return result;
}

static NSData *buffer_bytes(GLuint buffer)
{
	GLint prior = 0, size = 0;
	glGetIntegerv(GL_COPY_READ_BUFFER_BINDING, &prior);
	glBindBuffer(GL_COPY_READ_BUFFER, buffer);
	glGetBufferParameteriv(GL_COPY_READ_BUFFER, GL_BUFFER_SIZE, &size);
	NSData *data = nil;
	if (size > 0 && size <= 64 * 1024 * 1024) {
		const void *bytes = glMapBufferRange(GL_COPY_READ_BUFFER, 0, size, GL_MAP_READ_BIT);
		if (bytes) {
			data = [NSData dataWithBytes:bytes length:size];
			if (!glUnmapBuffer(GL_COPY_READ_BUFFER)) data = nil;
		}
	}
	glBindBuffer(GL_COPY_READ_BUFFER, (GLuint)prior);
	return data;
}

static void capture_draw_elements(GLenum mode, GLsizei count, GLenum type, const void *indices)
{
	static NSMutableDictionary *programs;
	static NSData *targets[2];
	static BOOL captured[2];
	static GLsizei target_count;
	static BOOL initialized;
	static long minimum = 3;
	if (!initialized) {
		initialized = YES;
		programs = [NSMutableDictionary dictionary];
		if (getenv("XG_CAPTURE_MIN_INDICES")) minimum = strtol(getenv("XG_CAPTURE_MIN_INDICES"), NULL, 10);
		if (minimum < 3 || minimum > 100000) minimum = 3;
		NSString *folder = @(getenv("XG_CAPTURE_SHADER_DIR") ?: "");
		targets[0] = [NSData dataWithContentsOfFile:[folder stringByAppendingPathComponent:@"vs017_0.glsl"]];
		targets[1] = [NSData dataWithContentsOfFile:[folder stringByAppendingPathComponent:@"vs041_0.glsl"]];
		xg_log("draw capture: targets loaded %lu / %lu bytes", (unsigned long)targets[0].length, (unsigned long)targets[1].length);
	}
	if (count < minimum || mode != GL_TRIANGLES || (captured[0] && captured[1])) {
		glDrawElements(mode, count, type, indices);
		return;
	}
	GLint program = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &program);
	NSNumber *known = programs[@(program)];
	GLuint shaders[8];
	GLsizei shader_count = 0;
	if (!known) {
		int target = -1;
		glGetAttachedShaders(program, 8, &shader_count, shaders);
		for (int i = 0; i < shader_count; i++) {
			GLint kind = 0;
			glGetShaderiv(shaders[i], GL_SHADER_TYPE, &kind);
			if (kind != GL_VERTEX_SHADER) continue;
			NSData *source = shader_source(shaders[i]);
			for (int j = 0; j < 2; j++) if (targets[j] && [source isEqualToData:targets[j]]) target = j;
		}
		known = @(target);
		programs[@(program)] = known;
		if (target >= 0) xg_log("draw capture: matched %s program %d", target ? "equal" : "base", program);
	}
	int target = known.intValue;
	if (target < 0 || captured[target]) goto draw;
	GLint depth_function = 0, depth_write = 0;
	glGetIntegerv(GL_DEPTH_FUNC, &depth_function);
	glGetIntegerv(GL_DEPTH_WRITEMASK, &depth_write);
	if ((!target && (depth_function != GL_LEQUAL || !depth_write)) ||
		(target && (!captured[0] || count != target_count || depth_function != GL_EQUAL || depth_write))) goto draw;
	@autoreleasepool {
		NSString *root = @(getenv("XG_DRAW_CAPTURE"));
		NSString *folder = [root stringByAppendingPathComponent:target ? @"equal" : @"base"];
		[NSFileManager.defaultManager createDirectoryAtPath:folder withIntermediateDirectories:YES attributes:nil error:nil];
		NSMutableDictionary *buffers = [NSMutableDictionary dictionary], *state = [NSMutableDictionary dictionary];
		NSMutableArray *attributes = [NSMutableArray array], *uniforms = [NSMutableArray array];
		BOOL complete = YES;
		GLint element = 0;
		glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &element);
		if (!element) complete = NO;
		state[@"mode"] = @(mode); state[@"count"] = @(count); state[@"index_type"] = @(type);
		state[@"program"] = @(program); state[@"captured_at"] = @(NSDate.date.timeIntervalSince1970);
		state[@"index_offset"] = @((uintptr_t)indices); state[@"element_buffer"] = @(element);
		for (int index = -1; index < 16; index++) {
			GLint buffer = element;
			if (index >= 0) {
				NSMutableDictionary *attribute = [NSMutableDictionary dictionary];
				const GLenum names[] = { GL_VERTEX_ATTRIB_ARRAY_ENABLED, GL_VERTEX_ATTRIB_ARRAY_SIZE,
					GL_VERTEX_ATTRIB_ARRAY_TYPE, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, GL_VERTEX_ATTRIB_ARRAY_STRIDE,
					GL_VERTEX_ATTRIB_ARRAY_INTEGER, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING };
				const char *keys[] = { "enabled", "size", "type", "normalized", "stride", "integer", "buffer" };
				for (int j = 0; j < 7; j++) {
					GLint value = 0; glGetVertexAttribiv(index, names[j], &value); attribute[@(keys[j])] = @(value);
				}
				void *pointer = NULL;
				GLfloat current[4];
				glGetVertexAttribPointerv(index, GL_VERTEX_ATTRIB_ARRAY_POINTER, &pointer);
				glGetVertexAttribfv(index, GL_CURRENT_VERTEX_ATTRIB, current);
				attribute[@"offset"] = @((uintptr_t)pointer); attribute[@"current_bits"] = float_bits(current, 4);
				[attributes addObject:attribute];
				buffer = [attribute[@"enabled"] boolValue] ? [attribute[@"buffer"] intValue] : 0;
			}
			if (buffer && !buffers[@(buffer).stringValue]) {
				NSData *bytes = buffer_bytes(buffer);
				NSString *file = [NSString stringWithFormat:@"buffer-%d.bin", buffer];
				if (!bytes || ![bytes writeToFile:[folder stringByAppendingPathComponent:file] atomically:YES]) complete = NO;
				else buffers[@(buffer).stringValue] = @{@"file":file, @"size":@(bytes.length)};
			}
		}
		for (int i = 0; i < 196; i++) {
			NSString *name = i < 192 ? [NSString stringWithFormat:@"c[%d]", i] :
				@[ @"viewport_scale", @"viewport_offset", @"point_size", @"screen_offset" ][i - 192];
			GLint location = glGetUniformLocation(program, name.UTF8String);
			if (location < 0) continue;
			GLfloat values[4] = { 0 };
			glGetUniformfv(program, location, values);
			[uniforms addObject:@{@"name":name, @"bits":float_bits(values, i >= 194 ? 1 : 4)}];
		}
		const GLenum names[] = { GL_DEPTH_FUNC, GL_DEPTH_WRITEMASK, GL_STENCIL_FUNC, GL_STENCIL_REF,
			GL_STENCIL_VALUE_MASK, GL_STENCIL_WRITEMASK, GL_STENCIL_FAIL, GL_STENCIL_PASS_DEPTH_FAIL, GL_STENCIL_PASS_DEPTH_PASS };
		for (int i = 0; i < 9; i++) { GLint value = 0; glGetIntegerv(names[i], &value); state[[NSString stringWithFormat:@"gl-%x", names[i]]] = @(value); }
		GLfloat range[2], offset[2], viewport[4];
		glGetFloatv(GL_DEPTH_RANGE, range); glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &offset[0]);
		glGetFloatv(GL_POLYGON_OFFSET_UNITS, &offset[1]); glGetFloatv(GL_VIEWPORT, viewport);
		state[@"depth_range_bits"] = float_bits(range, 2); state[@"polygon_offset_bits"] = float_bits(offset, 2);
		state[@"viewport_bits"] = float_bits(viewport, 4); state[@"polygon_offset_enabled"] = @(glIsEnabled(GL_POLYGON_OFFSET_FILL));
		for (NSNumber *capability in @[@(GL_DEPTH_TEST), @(GL_STENCIL_TEST), @(GL_CULL_FACE), @(GL_BLEND), @(GL_SCISSOR_TEST)])
			state[[NSString stringWithFormat:@"enabled-%x", capability.unsignedIntValue]] = @(glIsEnabled(capability.unsignedIntValue));
		for (NSNumber *attachment in @[@(GL_COLOR_ATTACHMENT0), @(GL_DEPTH_ATTACHMENT)]) {
			GLint kind = 0, bits = 0;
			glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachment.unsignedIntValue, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &kind);
			if (kind != GL_NONE) glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachment.unsignedIntValue,
				attachment.unsignedIntValue == GL_DEPTH_ATTACHMENT ? GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE : GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE, &bits);
			state[[NSString stringWithFormat:@"attachment-%x", attachment.unsignedIntValue]] = @{@"type":@(kind), @"bits":@(bits)};
		}
		glGetAttachedShaders(program, 8, &shader_count, shaders);
		for (int i = 0; i < shader_count; i++) {
			GLint kind = 0; glGetShaderiv(shaders[i], GL_SHADER_TYPE, &kind);
			NSData *source = shader_source(shaders[i]);
			if (![source writeToFile:[folder stringByAppendingPathComponent:kind == GL_VERTEX_SHADER ? @"vertex.glsl" : @"fragment.glsl"] atomically:YES]) complete = NO;
		}
		GLenum error = glGetError();
		state[@"attributes"] = attributes; state[@"uniforms"] = uniforms; state[@"buffers"] = buffers;
		state[@"complete"] = @(complete && error == GL_NO_ERROR); state[@"gl_error"] = @(error);
		NSData *json = [NSJSONSerialization dataWithJSONObject:state options:NSJSONWritingPrettyPrinted error:nil];
		[json writeToFile:[folder stringByAppendingPathComponent:@"draw.json"] atomically:YES];
		captured[target] = YES;
		if (!target) target_count = count;
		xg_log("draw capture: %s program %d count %d complete %d error 0x%x", target ? "equal" : "base", program, count, complete, error);
	}
draw:
	glDrawElements(mode, count, type, indices);
}

void *xg_draw_capture_proc(const char *name)
{
	return getenv("XG_DRAW_CAPTURE") && getenv("XG_CAPTURE_SHADER_DIR") && !strcmp(name, "glDrawElements") ? capture_draw_elements : NULL;
}
#endif
