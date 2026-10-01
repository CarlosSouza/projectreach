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
	NSMutableData *pixels = nil;
	const char *failure = "Unsupported depth target or viewport";
	if (kind != GL_TEXTURE || !depth || level != 0 || viewport[0] || viewport[1] || viewport[2] != 640 || viewport[3] != 480) goto restore;
	glBindTexture(GL_TEXTURE_2D, depth);
	GLint base = -1, red = 0;
	glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, &base);
	glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, &red);
	if (base != 0 || red != GL_RED) goto restore;
	const char *sources[] = {
		"#version 300 es\nvoid main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2.0-1.0,0,1);}\n",
		"#version 300 es\nprecision highp float; precision highp int; uniform highp sampler2D depth_image; out vec4 c;\n"
		"void main(){uint b=floatBitsToUint(texelFetch(depth_image,ivec2(gl_FragCoord.xy),0).r);"
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
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, 640, 480);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color);
	status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) { failure = "Depth observation target incomplete"; goto restore; }
	glGenSamplers(1, &point_sampler);
	glSamplerParameteri(point_sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glSamplerParameteri(point_sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glSamplerParameteri(point_sampler, GL_TEXTURE_COMPARE_MODE, GL_NONE); glBindSampler(0, point_sampler);
	glGenVertexArrays(1, &vertices); glBindVertexArray(vertices);
	for (int i = 0; i < sizeof(caps) / sizeof(caps[0]); i++) glDisable(caps[i]);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); glViewport(0, 0, 640, 480);
	glUseProgram(probe); glUniform1i(glGetUniformLocation(probe, "depth_image"), 0);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0); glPixelStorei(GL_PACK_ALIGNMENT, 1);
	for (int i = 2; i < 5; i++) glPixelStorei(pack_names[i], 0);
	pixels = [NSMutableData dataWithLength:640 * 480 * 4];
	glReadPixels(0, 0, 640, 480, GL_RGBA, GL_UNSIGNED_BYTE, pixels.mutableBytes);
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
	NSDictionary *metadata = @{@"complete":@(complete), @"file":file, @"encoding":@"normalized-float32-le", @"width":@640, @"height":@480,
		@"presented_frames":@(xg_draw_capture_frame()),
		@"calibrated":@(calibrated),
		@"framebuffer":@(draw), @"texture":@(depth), @"level":@(level), @"framebuffer_status":@(status),
		@"gl_error":@(error), @"restore_error":@(restore_error), @"error":@(failure)};
	[[NSJSONSerialization dataWithJSONObject:metadata options:NSJSONWritingPrettyPrinted error:nil]
		writeToFile:[folder stringByAppendingPathComponent:[@"depth-" stringByAppendingString:[label stringByAppendingString:@".json"]]] atomically:YES];
	xg_log("depth observation: %s texture %d complete %d error 0x%x restore 0x%x", label.UTF8String, depth, complete, error, restore_error);
}
#endif
