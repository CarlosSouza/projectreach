/* Opt-in Simulator depth observation, not a renderer workaround.
 * Encode normalized depth samples as float32 bits in a private RGBA8 target.
 * Never attach the game's depth texture to our draw target or write into it. */
#import <Foundation/Foundation.h>
#include <TargetConditionals.h>
#if TARGET_OS_SIMULATOR
#import <OpenGLES/ES3/gl.h>
#include <string.h>
#include "xg_host.h"
unsigned long xg_draw_capture_frame(void);
void xg_capture_depth(NSString *folder, NSString *label);

/* Read color without changing the draw, pixel-pack buffer or pack layout. */
NSData *xg_read_native_color(GLint source, GLenum *error)
{
	GLint read = 0, pack[5];
	const GLenum names[] = { GL_PIXEL_PACK_BUFFER_BINDING, GL_PACK_ALIGNMENT, GL_PACK_ROW_LENGTH, GL_PACK_SKIP_ROWS, GL_PACK_SKIP_PIXELS };
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read);
	for (int i = 0; i < 5; i++) glGetIntegerv(names[i], &pack[i]);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, source);
	GLint buffer = 0; glGetIntegerv(GL_READ_BUFFER, &buffer);
	NSMutableData *pixels = nil;
	if (buffer == GL_COLOR_ATTACHMENT0) {
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0); glPixelStorei(GL_PACK_ALIGNMENT, 1);
		for (int i = 2; i < 5; i++) glPixelStorei(names[i], 0);
		pixels = [NSMutableData dataWithLength:640 * 480 * 4];
		glReadPixels(0, 0, 640, 480, GL_RGBA, GL_UNSIGNED_BYTE, pixels.mutableBytes);
	}
	glBindBuffer(GL_PIXEL_PACK_BUFFER, pack[0]);
	for (int i = 1; i < 5; i++) glPixelStorei(names[i], pack[i]);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, read);
	*error = glGetError();
	return *error == GL_NO_ERROR ? pixels : nil;
}

/* Original linked program, VAO, uniforms, textures and pixel state. Only the
 * target is replaced by owned, same-format textures. EQUAL/ALWAYS repeats
 * cannot write game color/depth/stencil. Color differences are NOT coverage. */
void xg_capture_native_pixels(NSString *folder, GLenum mode, GLsizei count, GLenum type, const void *indices)
{
	GLint draw = 0, read = 0, renderbuffer = 0, function = 0, viewport[4], samples = 0;
	GLint program = 0, vao = 0, depth_kind = 0, depth_object = 0, stencil_object = 0;
	GLint texture = 0, unpack_buffer = 0;
	GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST), mask = GL_TRUE;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw); glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read);
	glGetIntegerv(GL_RENDERBUFFER_BINDING, &renderbuffer); glGetIntegerv(GL_DEPTH_FUNC, &function);
	glGetIntegerv(GL_VIEWPORT, viewport); glGetIntegerv(GL_SAMPLES, &samples);
	glGetIntegerv(GL_CURRENT_PROGRAM, &program); glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
	glGetBooleanv(GL_DEPTH_WRITEMASK, &mask);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture); glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &unpack_buffer);
	GLuint target = 0, color = 0, depth = 0;
	BOOL complete = NO, copied = NO;
	NSData *before = nil;
	GLenum error = GL_NO_ERROR, status = 0;
	const char *failure = "Unsupported native pixel target/state";
	if (!draw || viewport[0] || viewport[1] || viewport[2] != 640 || viewport[3] != 480 || samples ||
		function != GL_EQUAL || mask || !glIsEnabled(GL_DEPTH_TEST) || glIsEnabled(GL_RASTERIZER_DISCARD)) goto restore_native;
	/* Reject MRT, non-RGBA8, sRGB, non-D24S8 and active occlusion queries. */
	GLint maximum = 0; glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maximum);
	for (int i = 0; i < maximum; i++) {
		GLint buffer = 0; glGetIntegerv(GL_DRAW_BUFFER0 + i, &buffer);
		if (buffer != (i ? GL_NONE : GL_COLOR_ATTACHMENT0)) goto restore_native;
	}
	for (NSNumber *parameter in @[@(GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE), @(GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE),
		@(GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE), @(GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE), @(GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING)]) {
		GLint value = 0;
		glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, parameter.unsignedIntValue, &value);
		if (value != (parameter.unsignedIntValue == GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING ? GL_LINEAR : 8)) goto restore_native;
	}
	GLint bits = 0;
	glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &bits);
	if (bits != GL_TEXTURE) goto restore_native;
	glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &bits);
	if (bits != 24) goto restore_native;
	glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE, &bits);
	if (bits != 8) goto restore_native;
	glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &depth_kind);
	glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &depth_object);
	glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &stencil_object);
	if (depth_kind != GL_TEXTURE || !depth_object || depth_object != stencil_object) goto restore_native;
	for (NSNumber *query in @[@(GL_ANY_SAMPLES_PASSED), @(GL_ANY_SAMPLES_PASSED_CONSERVATIVE), @(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN)]) {
		GLint current = 0; glGetQueryiv(query.unsignedIntValue, GL_CURRENT_QUERY, &current);
		if (current) goto restore_native;
	}
	before = xg_read_native_color(draw, &error);
	if (!before || error) goto restore_native;
	glGenFramebuffers(1, &target); glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	glGenTextures(1, &color); glBindTexture(GL_TEXTURE_2D, color);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 640, 480, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
	glGenTextures(1, &depth); glBindTexture(GL_TEXTURE_2D, depth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, 640, 480, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depth, 0);
	glBindTexture(GL_TEXTURE_2D, texture); glBindBuffer(GL_PIXEL_UNPACK_BUFFER, unpack_buffer);
	status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) goto restore_native;
	complete = [before writeToFile:[folder stringByAppendingPathComponent:@"native-before.rgba"] atomically:YES];
	for (int pass = 0; pass < 3; pass++) {
		glBindFramebuffer(GL_READ_FRAMEBUFFER, draw); glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target);
		glDisable(GL_SCISSOR_TEST);
		glBlitFramebuffer(0, 0, 640, 480, 0, 0, 640, 480,
			GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);
		if (scissor) glEnable(GL_SCISSOR_TEST);
		if (!pass) {
			NSData *initial = xg_read_native_color(target, &error);
			copied = initial && [initial isEqualToData:before] && !error;
			if (!copied) { complete = NO; break; }
			xg_capture_depth(folder, @"native-copy");
		}
		glDepthFunc(pass == 1 ? GL_ALWAYS : function);
		glDrawElements(mode, count, type, indices);
		NSData *pixels = xg_read_native_color(target, &error);
		NSString *name = @[ @"native-equal.rgba", @"native-always.rgba", @"native-repeat.rgba" ][pass];
		complete &= pixels && !error && [pixels writeToFile:[folder stringByAppendingPathComponent:name] atomically:YES];
		if (!complete) break;
	}
	failure = complete ? "" : "Native pixel copy/draw/read failed";
restore_native:
	if (!error) error = glGetError();
	glDepthFunc(function); glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw); glBindFramebuffer(GL_READ_FRAMEBUFFER, read);
	glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
	glBindTexture(GL_TEXTURE_2D, texture); glBindBuffer(GL_PIXEL_UNPACK_BUFFER, unpack_buffer);
	if (scissor) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
	glDeleteFramebuffers(1, &target); glDeleteTextures(1, &color); glDeleteTextures(1, &depth);
	GLenum restore_error = glGetError();
	complete &= copied && error == GL_NO_ERROR && restore_error == GL_NO_ERROR;
	NSDictionary *record = @{@"complete":@(complete), @"color_copy_equal":@(copied), @"framebuffer_status":@(status),
		@"gl_error":@(error), @"restore_error":@(restore_error), @"error":@(failure), @"framebuffer":@(draw),
		@"depth_texture":@(depth_object), @"program":@(program), @"vao":@(vao), @"presented_frames":@(xg_draw_capture_frame()),
		@"width":@640, @"height":@480, @"depth_bits":@24, @"stencil_bits":@8};
	[[NSJSONSerialization dataWithJSONObject:record options:NSJSONWritingPrettyPrinted error:nil]
		writeToFile:[folder stringByAppendingPathComponent:@"native-pixels.json"] atomically:YES];
	xg_log("native pixels: complete %d error 0x%x restore 0x%x", complete, error, restore_error);
}

void xg_capture_native_after(NSString *folder)
{
	GLint draw = 0; glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
	GLenum error = GL_NO_ERROR;
	NSData *pixels = xg_read_native_color(draw, &error);
	BOOL okay = pixels && [pixels writeToFile:[folder stringByAppendingPathComponent:@"native-live.rgba"] atomically:YES];
	NSDictionary *record = @{@"complete":@(okay && !error), @"gl_error":@(error), @"framebuffer":@(draw),
		@"presented_frames":@(xg_draw_capture_frame())};
	[[NSJSONSerialization dataWithJSONObject:record options:NSJSONWritingPrettyPrinted error:nil]
		writeToFile:[folder stringByAppendingPathComponent:@"native-live.json"] atomically:YES];
}

void xg_capture_depth(NSString *folder, NSString *label)
{
	const GLenum caps[] = { GL_DEPTH_TEST, GL_STENCIL_TEST, GL_BLEND, GL_CULL_FACE, GL_SCISSOR_TEST,
		GL_DITHER, GL_RASTERIZER_DISCARD, GL_SAMPLE_ALPHA_TO_COVERAGE, GL_SAMPLE_COVERAGE };
	GLboolean enabled[sizeof(caps) / sizeof(caps[0])], color_mask[4];
	GLint draw = 0, read = 0, viewport[4], program = 0, vao = 0, renderbuffer = 0;
	GLint active = 0, texture = 0, sampler = 0, pack[5], depth = 0, kind = 0, level = -1;
	const GLenum pack_names[] = { GL_PIXEL_PACK_BUFFER_BINDING, GL_PACK_ALIGNMENT, GL_PACK_ROW_LENGTH, GL_PACK_SKIP_ROWS, GL_PACK_SKIP_PIXELS };
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw); glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read);
	glGetIntegerv(GL_VIEWPORT, viewport); glGetIntegerv(GL_CURRENT_PROGRAM, &program);
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao); glGetIntegerv(GL_RENDERBUFFER_BINDING, &renderbuffer);
	glGetBooleanv(GL_COLOR_WRITEMASK, color_mask);
	for (int i = 0; i < sizeof(caps) / sizeof(caps[0]); i++) enabled[i] = glIsEnabled(caps[i]);
	for (int i = 0; i < 5; i++) glGetIntegerv(pack_names[i], &pack[i]);
	glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &kind);
	if (kind == GL_TEXTURE) {
		glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &depth);
		glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL, &level);
	}
	glGetIntegerv(GL_ACTIVE_TEXTURE, &active); glActiveTexture(GL_TEXTURE0);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture); glGetIntegerv(GL_SAMPLER_BINDING, &sampler);
	GLuint target = 0, color = 0, vertices = 0, point_sampler = 0, probe = 0, shaders[2] = { 0, 0 };
	GLenum status = 0, error = 0;
	BOOL complete = NO, calibrated = NO;
	GLsizei width = viewport[2], height = viewport[3];
	uint32_t texture_extent = 0;
	NSMutableData *pixels = nil;
	const char *failure = "Unsupported depth target or viewport";
	if (kind != GL_TEXTURE || !depth || level != 0 || viewport[0] || viewport[1] ||
		width < 4 || height < 1 || width > 4096 || height > 4096) goto restore;
	glBindTexture(GL_TEXTURE_2D, depth);
	GLint base = -1, red = 0;
	glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, &base);
	glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, &red);
	if (base != 0 || red != GL_RED) goto restore;
	const char *sources[] = {
		"#version 300 es\nvoid main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2.0-1.0,0,1);}\n",
		"#version 300 es\nprecision highp float; precision highp int; uniform highp sampler2D depth_image; uniform bool dimensions_only; out vec4 c;\n"
		"void main(){uvec2 s=uvec2(textureSize(depth_image,0)); uint b=dimensions_only ? (s.x | (s.y<<16)) : floatBitsToUint(texelFetch(depth_image,ivec2(gl_FragCoord.xy),0).r);"
		"c=vec4(float(b&255u),float((b>>8)&255u),float((b>>16)&255u),float(b>>24))/255.0;}\n"
	};
	for (int i = 0; i < 2; i++) {
		GLint okay = 0;
		shaders[i] = glCreateShader(i ? GL_FRAGMENT_SHADER : GL_VERTEX_SHADER);
		glShaderSource(shaders[i], 1, &sources[i], NULL); glCompileShader(shaders[i]);
		glGetShaderiv(shaders[i], GL_COMPILE_STATUS, &okay);
		if (!okay) { failure = "Depth observation shader failed"; goto restore; }
	}
	probe = glCreateProgram(); glAttachShader(probe, shaders[0]); glAttachShader(probe, shaders[1]); glLinkProgram(probe);
	GLint linked = 0; glGetProgramiv(probe, GL_LINK_STATUS, &linked);
	if (!linked) { failure = "Depth observation program failed"; goto restore; }
	glGenFramebuffers(1, &target); glBindFramebuffer(GL_FRAMEBUFFER, target);
	glGenRenderbuffers(1, &color); glBindRenderbuffer(GL_RENDERBUFFER, color);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color);
	status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) { failure = "Depth observation target incomplete"; goto restore; }
	glGenSamplers(1, &point_sampler);
	glSamplerParameteri(point_sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glSamplerParameteri(point_sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glSamplerParameteri(point_sampler, GL_TEXTURE_COMPARE_MODE, GL_NONE); glBindSampler(0, point_sampler);
	glGenVertexArrays(1, &vertices); glBindVertexArray(vertices);
	for (int i = 0; i < sizeof(caps) / sizeof(caps[0]); i++) glDisable(caps[i]);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glUseProgram(probe); glUniform1i(glGetUniformLocation(probe, "depth_image"), 0);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0); glPixelStorei(GL_PACK_ALIGNMENT, 1);
	for (int i = 2; i < 5; i++) glPixelStorei(pack_names[i], 0);
	/* ES 3.0 lacks glGetTexLevelParameteriv. Ask the sampler for the actual
	 * extent before reading depth; a larger viewport must not certify a crop
	 * or out-of-range texelFetch as a full attachment observation. */
	GLint dimensions = glGetUniformLocation(probe, "dimensions_only");
	glUniform1i(dimensions, 1); glViewport(0, 0, 1, 1); glDrawArrays(GL_TRIANGLES, 0, 3);
	glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &texture_extent);
	if ((texture_extent & 0xffff) != width || (texture_extent >> 16) != height) {
		failure = "Depth texture extent differs from viewport"; goto restore;
	}
	glUniform1i(dimensions, 0); glViewport(0, 0, width, height); glDrawArrays(GL_TRIANGLES, 0, 3);
	pixels = [NSMutableData dataWithLength:(NSUInteger)width * height * 4];
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.mutableBytes);
	/* Verify the sampler/float-bits/byte-packing path with exact binary fractions.
	 * The calibration uses only an owned texture and the owned draw target. */
	GLint unpack[5];
	const GLenum unpack_names[] = { GL_PIXEL_UNPACK_BUFFER_BINDING, GL_UNPACK_ALIGNMENT, GL_UNPACK_ROW_LENGTH, GL_UNPACK_SKIP_ROWS, GL_UNPACK_SKIP_PIXELS };
	for (int i = 0; i < 5; i++) glGetIntegerv(unpack_names[i], &unpack[i]);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0); glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	for (int i = 2; i < 5; i++) glPixelStorei(unpack_names[i], 0);
	GLuint reference = 0; glGenTextures(1, &reference); glBindTexture(GL_TEXTURE_2D, reference);
	const GLfloat samples[] = { .25f, .5f, .75f, 1.f };
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, 4, 1, 0, GL_DEPTH_COMPONENT, GL_FLOAT, samples);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glViewport(0, 0, 4, 1); glDrawArrays(GL_TRIANGLES, 0, 3);
	uint32_t observed[4] = { 0 };
	glReadPixels(0, 0, 4, 1, GL_RGBA, GL_UNSIGNED_BYTE, observed);
	calibrated = !memcmp(observed, samples, sizeof(samples));
	glDeleteTextures(1, &reference);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, unpack[0]);
	for (int i = 1; i < 5; i++) glPixelStorei(unpack_names[i], unpack[i]);
	error = glGetError(); complete = error == GL_NO_ERROR;
	complete &= calibrated;
	failure = error ? "Depth observation GL error" : calibrated ? "" : "Depth observation calibration mismatch";
restore:
	if (!error) error = glGetError();
	glUseProgram(program); glBindVertexArray(vao);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw); glBindFramebuffer(GL_READ_FRAMEBUFFER, read);
	glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
	glBindTexture(GL_TEXTURE_2D, texture); glBindSampler(0, sampler); glActiveTexture(active);
	glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
	glColorMask(color_mask[0], color_mask[1], color_mask[2], color_mask[3]);
	for (int i = 0; i < sizeof(caps) / sizeof(caps[0]); i++) if (enabled[i]) glEnable(caps[i]); else glDisable(caps[i]);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, pack[0]);
	for (int i = 1; i < 5; i++) glPixelStorei(pack_names[i], pack[i]);
	if (probe) glDeleteProgram(probe); for (int i = 0; i < 2; i++) if (shaders[i]) glDeleteShader(shaders[i]);
	glDeleteVertexArrays(1, &vertices); glDeleteSamplers(1, &point_sampler);
	glDeleteFramebuffers(1, &target); glDeleteRenderbuffers(1, &color);
	GLenum restore_error = glGetError();
	NSString *file = [@"depth-" stringByAppendingString:[label stringByAppendingString:@".bin"]];
	complete &= error == GL_NO_ERROR && restore_error == GL_NO_ERROR;
	if (complete) complete = [pixels writeToFile:[folder stringByAppendingPathComponent:file] atomically:YES];
	NSDictionary *metadata = @{@"complete":@(complete), @"file":file, @"encoding":@"normalized-float32-le", @"width":@(width), @"height":@(height),
		@"capture_schema":@2, @"texture_width":@(texture_extent & 0xffff), @"texture_height":@(texture_extent >> 16),
		@"presented_frames":@(xg_draw_capture_frame()),
		@"calibrated":@(calibrated),
		@"framebuffer":@(draw), @"texture":@(depth), @"level":@(level), @"framebuffer_status":@(status),
		@"gl_error":@(error), @"restore_error":@(restore_error), @"error":@(failure)};
	[[NSJSONSerialization dataWithJSONObject:metadata options:NSJSONWritingPrettyPrinted error:nil]
		writeToFile:[folder stringByAppendingPathComponent:[@"depth-" stringByAppendingString:[label stringByAppendingString:@".json"]]] atomically:YES];
	xg_log("depth observation: %s texture %d complete %d error 0x%x restore 0x%x", label.UTF8String, depth, complete, error, restore_error);
}
#endif
