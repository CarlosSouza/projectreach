/* Opt-in Simulator diagnostics. Captured game-derived data stays in ref/.
 * Observe existing draws without relinking or modifying the guest's GL state. */
#import <Foundation/Foundation.h>
#include <TargetConditionals.h>
#if TARGET_OS_SIMULATOR
#import <OpenGLES/ES3/gl.h>
#include <stdlib.h>
#include <string.h>
#include <mach/mach.h>
#include "xg_host.h"

static NSMutableDictionary *texture_sizes;
static NSMutableDictionary *texture_uploads;
static NSMutableDictionary *texture_sources;
void xg_capture_depth(NSString *folder, NSString *label);
void xg_capture_native_pixels(NSString *folder, GLenum mode, GLsizei count, GLenum type, const void *indices);
void xg_capture_native_after(NSString *folder);
static unsigned long presented_frames;
void xg_draw_capture_present(void) { presented_frames++; }
unsigned long xg_draw_capture_frame(void) { return presented_frames; }

/* arm64_32 texture_entry ABI in the pinned upstream xbox_textures.c. Reads
 * fail closed; never call guest functions or dereference an unchecked node. */
static NSDictionary *texture_source(GLuint texture, GLsizei width, GLsizei height)
{
	const char *setting = getenv("XG_TEXTURE_BUCKETS");
	if (!setting) return nil;
	char *end = NULL;
	uint64_t buckets = strtoull(setting, &end, 16);
	if (!end || *end || !xg_header || buckets < XG_IMAGE_BASE || buckets > UINT32_MAX || buckets + 4096 * 4 > xg_header->image_end)
		return @{@"supported":@NO, @"error":@"Invalid texture cache symbol"};
	_Static_assert(sizeof(vm_address_t) == sizeof(uintptr_t), "Host memory reads require full-width addresses");
	uint32_t heads[4096]; vm_size_t read = 0;
	if (vm_read_overwrite(mach_task_self(), G(vm_address_t, buckets), sizeof(heads),
		(vm_address_t)heads, &read) != KERN_SUCCESS || read != sizeof(heads))
		return @{@"supported":@NO, @"error":@"Unreadable texture cache"};
	unsigned int visited = 0;
	for (int bucket = 0; bucket < 4096; bucket++) for (uint32_t node = heads[bucket]; node;) {
		uint32_t entry[20]; read = 0;
		if (++visited > 16384 || node < 0x10000000u || (uint64_t)node + sizeof(entry) > 0xfff00000u ||
			vm_read_overwrite(mach_task_self(), G(vm_address_t, node), sizeof(entry),
				(vm_address_t)entry, &read) != KERN_SUCCESS || read != sizeof(entry))
			return @{@"supported":@NO, @"error":@"Invalid texture cache node"};
		node = entry[0];
		if (entry[5] != texture) continue;
		uint32_t format = entry[7], w = entry[8], h = entry[9], depth = entry[10], linear = entry[13];
		if (format != 0x05 && format != 0x11) return @{@"supported":@NO, @"format":@(format)};
		uint64_t pitch = linear ? entry[15] : w * 2ull;
		uint64_t length = pitch * h, address = entry[16];
		if (entry[6] != GL_TEXTURE_2D || w != width || h != height || depth != 1 || entry[12] || entry[14] ||
			((entry[2] >> 8) & 0xff) != format || address != (entry[1] | XG_WINDOW_BASE) ||
			pitch < w * 2ull || length > 32 * 1024 * 1024 || length > entry[17] ||
			address < XG_WINDOW_BASE || address + length > (uint64_t)XG_WINDOW_BASE + XG_WINDOW_SIZE)
			return @{@"supported":@NO, @"error":@"Unsupported texture cache layout"};
		NSData *bytes = [NSData dataWithBytes:G(void *, address) length:(NSUInteger)length];
		return @{@"supported":@YES, @"format":@(format), @"width":@(w), @"height":@(h),
			@"linear":@(linear), @"pitch":@(pitch), @"length":@(length), @"data":@(entry[1]),
			@"format_word":@(entry[2]), @"size_word":@(entry[3]), @"bytes":bytes};
	}
	return @{@"supported":@NO, @"error":@"Texture absent from Xbox cache"};
}

static void observe_texture_image(GLenum target, GLint level, GLint format, GLsizei width, GLsizei height,
	GLint border, GLenum pixels, GLenum type, const void *data)
{
	glTexImage2D(target, level, format, width, height, border, pixels, type, data);
	if (target != GL_TEXTURE_2D || level != 0) return;
	GLint texture = 0; glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
	if (!texture_sizes) texture_sizes = [NSMutableDictionary dictionary];
	texture_sizes[@(texture)] = @[@(width), @(height)];
	if (!texture_uploads) texture_uploads = [NSMutableDictionary dictionary];
	[texture_uploads removeObjectForKey:@(texture)];
	NSDictionary *source = texture_source(texture, width, height);
	if (source) {
		if (!texture_sources) texture_sources = [NSMutableDictionary dictionary];
		texture_sources[@(texture)] = source;
	}
	GLint unpack[5];
	const GLenum names[] = { GL_PIXEL_UNPACK_BUFFER_BINDING, GL_UNPACK_ROW_LENGTH, GL_UNPACK_SKIP_ROWS, GL_UNPACK_SKIP_PIXELS, GL_UNPACK_ALIGNMENT };
	for (int i = 0; i < 5; i++) glGetIntegerv(names[i], &unpack[i]);
	if (data && pixels == GL_RGBA && type == GL_UNSIGNED_BYTE && width > 0 && height > 0 && width <= 4096 && height <= 4096 &&
		!unpack[0] && !unpack[1] && !unpack[2] && !unpack[3] && unpack[4] > 0 && ((width * 4) % unpack[4]) == 0)
		texture_uploads[@(texture)] = [NSData dataWithBytes:data length:(NSUInteger)width * height * 4];
}

static void observe_texture_update(GLenum target, GLint level, GLint x, GLint y, GLsizei width, GLsizei height,
	GLenum format, GLenum type, const void *data)
{
	glTexSubImage2D(target, level, x, y, width, height, format, type, data);
	if (target == GL_TEXTURE_2D && level == 0) {
		GLint texture = 0; glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
		[texture_uploads removeObjectForKey:@(texture)];
		[texture_sources removeObjectForKey:@(texture)];
	}
}

/* Read a separate FBO, never draw into or change the captured texture. */
static NSDictionary *read_texture(GLuint texture, NSString *folder)
{
	NSArray *size = texture_sizes[@(texture)];
	GLsizei width = [size[0] intValue], height = [size[1] intValue];
	if (!size || width <= 0 || height <= 0 || width > 4096 || height > 4096)
		return @{@"complete":@NO, @"error":@"Missing or unsupported texture dimensions"};
	GLint read = 0, pack[5];
	const GLenum names[] = { GL_PIXEL_PACK_BUFFER_BINDING, GL_PACK_ALIGNMENT, GL_PACK_ROW_LENGTH, GL_PACK_SKIP_ROWS, GL_PACK_SKIP_PIXELS };
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read);
	for (int i = 0; i < 5; i++) glGetIntegerv(names[i], &pack[i]);
	GLuint framebuffer = 0; glGenFramebuffers(1, &framebuffer); glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
	glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
	GLenum status = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
	NSMutableData *rgba = [NSMutableData dataWithLength:(NSUInteger)width * height * 4];
	if (status == GL_FRAMEBUFFER_COMPLETE) {
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0); glPixelStorei(GL_PACK_ALIGNMENT, 1);
		for (int i = 2; i < 5; i++) glPixelStorei(names[i], 0);
		glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgba.mutableBytes);
	}
	GLenum error = glGetError();
	glBindFramebuffer(GL_READ_FRAMEBUFFER, read); glDeleteFramebuffers(1, &framebuffer);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, pack[0]);
	for (int i = 1; i < 5; i++) glPixelStorei(names[i], pack[i]);
	NSString *file = [NSString stringWithFormat:@"texture-%u.rgba", texture], *preview = [NSString stringWithFormat:@"texture-%u.ppm", texture];
	NSString *upload_file = [NSString stringWithFormat:@"texture-%u.upload.rgba", texture];
	NSData *upload = texture_uploads[@(texture)];
	BOOL complete = status == GL_FRAMEBUFFER_COMPLETE && error == GL_NO_ERROR;
	if (complete) {
		complete = [rgba writeToFile:[folder stringByAppendingPathComponent:file] atomically:YES];
		NSMutableData *ppm = [NSMutableData dataWithData:[[NSString stringWithFormat:@"P6\n%d %d\n255\n", width, height] dataUsingEncoding:NSASCIIStringEncoding]];
		const unsigned char *bytes = rgba.bytes;
		for (NSUInteger i = 0; i < (NSUInteger)width * height; i++) [ppm appendBytes:bytes + i * 4 length:3];
		complete &= [ppm writeToFile:[folder stringByAppendingPathComponent:preview] atomically:YES];
		if (upload) complete &= [upload writeToFile:[folder stringByAppendingPathComponent:upload_file] atomically:YES];
	}
	NSMutableDictionary *result = [NSMutableDictionary dictionaryWithDictionary:@{@"complete":@(complete), @"width":@(width), @"height":@(height), @"file":file, @"preview":preview,
		@"framebuffer_status":@(status), @"gl_error":@(error), @"upload_compared":@(upload != nil),
		@"upload_equal":@([rgba isEqualToData:upload]), @"upload_file":upload ? upload_file : @""}];
	if (texture_sources[@(texture)]) {
		NSMutableDictionary *source = [texture_sources[@(texture)] mutableCopy];
		NSData *bytes = source[@"bytes"]; [source removeObjectForKey:@"bytes"];
		if (bytes) {
			NSString *raw = [NSString stringWithFormat:@"texture-%u.xbox", texture];
			source[@"file"] = raw;
			source[@"complete"] = @([bytes writeToFile:[folder stringByAppendingPathComponent:raw] atomically:YES]);
			result[@"complete"] = @([result[@"complete"] boolValue] && [source[@"complete"] boolValue]);
		}
		result[@"xbox_source"] = source;
	}
	return result;
}

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
	int selected_target = -1;
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
		selected_target = target;
		if (getenv("XG_CAPTURE_DEPTH")) xg_capture_depth(folder, @"before");
		NSMutableDictionary *buffers = [NSMutableDictionary dictionary], *state = [NSMutableDictionary dictionary];
		NSMutableArray *attributes = [NSMutableArray array], *uniforms = [NSMutableArray array];
		BOOL complete = YES;
		GLint element = 0;
		glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &element);
		if (!element) complete = NO;
		state[@"mode"] = @(mode); state[@"count"] = @(count); state[@"index_type"] = @(type);
		state[@"program"] = @(program); state[@"captured_at"] = @(NSDate.date.timeIntervalSince1970);
		state[@"presented_frames"] = @(presented_frames);
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
		GLint draw_target = 0, read_target = 0, scissor[4];
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw_target); glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read_target);
		glGetIntegerv(GL_SCISSOR_BOX, scissor);
		state[@"framebuffer"] = @{@"draw":@(draw_target), @"read":@(read_target)};
		state[@"scissor"] = @[@(scissor[0]), @(scissor[1]), @(scissor[2]), @(scissor[3])];
		for (NSNumber *parameter in @[@(GL_CULL_FACE_MODE), @(GL_FRONT_FACE), @(GL_BLEND_SRC_RGB), @(GL_BLEND_DST_RGB),
			@(GL_BLEND_SRC_ALPHA), @(GL_BLEND_DST_ALPHA), @(GL_BLEND_EQUATION_RGB), @(GL_BLEND_EQUATION_ALPHA)]) {
			GLint value = 0; glGetIntegerv(parameter.unsignedIntValue, &value);
			state[[NSString stringWithFormat:@"gl-%x", parameter.unsignedIntValue]] = @(value);
		}
		for (NSNumber *capability in @[@(GL_DEPTH_TEST), @(GL_STENCIL_TEST), @(GL_CULL_FACE), @(GL_BLEND), @(GL_SCISSOR_TEST)])
			state[[NSString stringWithFormat:@"enabled-%x", capability.unsignedIntValue]] = @(glIsEnabled(capability.unsignedIntValue));
		for (NSNumber *attachment in @[@(GL_COLOR_ATTACHMENT0), @(GL_DEPTH_ATTACHMENT)]) {
			GLint kind = 0, bits = 0, object = 0;
			glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachment.unsignedIntValue, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &kind);
			if (kind != GL_NONE) {
				glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachment.unsignedIntValue,
					attachment.unsignedIntValue == GL_DEPTH_ATTACHMENT ? GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE : GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE, &bits);
				glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachment.unsignedIntValue, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &object);
			}
			state[[NSString stringWithFormat:@"attachment-%x", attachment.unsignedIntValue]] = @{@"type":@(kind), @"bits":@(bits), @"object":@(object)};
		}
		GLint active = 0; glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
		NSMutableArray *texture_units = [NSMutableArray array];
		for (int unit = 0; unit < 4; unit++) {
			glActiveTexture(GL_TEXTURE0 + unit);
			GLint sampler = 0; glGetIntegerv(GL_SAMPLER_BINDING, &sampler);
			NSMutableDictionary *record = [NSMutableDictionary dictionaryWithDictionary:@{@"unit":@(unit), @"sampler":@(sampler)}];
			for (NSNumber *parameter in @[@(GL_TEXTURE_MIN_FILTER), @(GL_TEXTURE_MAG_FILTER), @(GL_TEXTURE_WRAP_S),
				@(GL_TEXTURE_WRAP_T), @(GL_TEXTURE_WRAP_R), @(GL_TEXTURE_COMPARE_MODE), @(GL_TEXTURE_COMPARE_FUNC)]) {
				if (!sampler) break;
				GLint value = 0; glGetSamplerParameteriv(sampler, parameter.unsignedIntValue, &value);
				record[[NSString stringWithFormat:@"sampler-%x", parameter.unsignedIntValue]] = @(value);
			}
			for (NSNumber *parameter in @[@(GL_TEXTURE_MIN_LOD), @(GL_TEXTURE_MAX_LOD)]) {
				if (!sampler) break;
				GLfloat value = 0; glGetSamplerParameterfv(sampler, parameter.unsignedIntValue, &value);
				record[[NSString stringWithFormat:@"sampler-%x-bits", parameter.unsignedIntValue]] = float_bits(&value, 1);
			}
			const GLenum targets[] = { GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_3D };
			const GLenum bindings[] = { GL_TEXTURE_BINDING_2D, GL_TEXTURE_BINDING_CUBE_MAP, GL_TEXTURE_BINDING_3D };
			for (int target = 0; target < 3; target++) {
				GLint object = 0; glGetIntegerv(bindings[target], &object);
				NSMutableDictionary *texture = [NSMutableDictionary dictionaryWithDictionary:@{@"object":@(object)}];
				if (object) for (NSNumber *parameter in @[@(GL_TEXTURE_BASE_LEVEL), @(GL_TEXTURE_MAX_LEVEL),
					@(GL_TEXTURE_SWIZZLE_R), @(GL_TEXTURE_SWIZZLE_G), @(GL_TEXTURE_SWIZZLE_B), @(GL_TEXTURE_SWIZZLE_A)]) {
					GLint value = 0; glGetTexParameteriv(targets[target], parameter.unsignedIntValue, &value);
					texture[[NSString stringWithFormat:@"gl-%x", parameter.unsignedIntValue]] = @(value);
				}
				if (object && targets[target] == GL_TEXTURE_2D && getenv("XG_CAPTURE_TEXTURES")) {
					NSDictionary *image = read_texture(object, folder);
					texture[@"level0"] = image;
					complete &= [image[@"complete"] boolValue];
				}
				record[[NSString stringWithFormat:@"texture-%x", targets[target]]] = texture;
			}
			char name[16]; snprintf(name, sizeof(name), "tex%d", unit);
			GLint location = glGetUniformLocation(program, name), assigned = -1;
			if (location >= 0) glGetUniformiv(program, location, &assigned);
			record[@"uniform_unit"] = @(assigned);
			[texture_units addObject:record];
		}
		glActiveTexture(active); state[@"active_texture"] = @(active); state[@"texture_units"] = texture_units;
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
		if (target && getenv("XG_CAPTURE_NATIVE_PIXELS")) xg_capture_native_pixels(folder, mode, count, type, indices);
	}
draw:
	glDrawElements(mode, count, type, indices);
	if (selected_target == 1 && getenv("XG_CAPTURE_NATIVE_PIXELS"))
		xg_capture_native_after([@(getenv("XG_DRAW_CAPTURE")) stringByAppendingPathComponent:@"equal"]);
	if (selected_target >= 0 && getenv("XG_CAPTURE_DEPTH")) {
		NSString *folder = [@(getenv("XG_DRAW_CAPTURE")) stringByAppendingPathComponent:selected_target ? @"equal" : @"base"];
		xg_capture_depth(folder, @"after");
	}
}

void *xg_draw_capture_proc(const char *name)
{
	if (!getenv("XG_DRAW_CAPTURE") || !getenv("XG_CAPTURE_SHADER_DIR")) return NULL;
	if (!strcmp(name, "glDrawElements")) return capture_draw_elements;
	if (getenv("XG_CAPTURE_TEXTURES") && !strcmp(name, "glTexImage2D")) return observe_texture_image;
	if (getenv("XG_CAPTURE_TEXTURES") && !strcmp(name, "glTexSubImage2D")) return observe_texture_update;
	return NULL;
}
#endif
