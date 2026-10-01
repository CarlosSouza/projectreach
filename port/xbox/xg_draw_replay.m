/* Private, opt-in Simulator replay before guest GL state is initialized.
 * Transform feedback changes linkage: numeric evidence, not original-program
 * fidelity proof. Never uses or changes the player's saves. */
#import <Foundation/Foundation.h>
#include <TargetConditionals.h>
#if TARGET_OS_SIMULATOR
#import <OpenGLES/ES3/gl.h>
#include "xg_host.h"

static BOOL replay_draw(NSString *input, NSString *output, NSString *label)
{
	NSString *folder = [input stringByAppendingPathComponent:label];
	NSData *json = [NSData dataWithContentsOfFile:[folder stringByAppendingPathComponent:@"draw.json"]];
	if (!json) return NO;
	NSDictionary *draw = [NSJSONSerialization JSONObjectWithData:json options:0 error:nil];
	if (![draw isKindOfClass:NSDictionary.class] || [draw[@"attributes"] count] != 16) return NO;
	NSString *vertex = [NSString stringWithContentsOfFile:[folder stringByAppendingPathComponent:@"vertex.glsl"] encoding:NSUTF8StringEncoding error:nil];
	NSString *fragment = [NSString stringWithContentsOfFile:[folder stringByAppendingPathComponent:@"fragment.glsl"] encoding:NSUTF8StringEncoding error:nil];
	NSMutableArray *allocated = [NSMutableArray array];
	NSMutableDictionary *buffers = [NSMutableDictionary dictionary];
	NSData *positions = nil;
	NSData *elements = nil;
	GLuint shaders[2] = { 0 }, program = 0, vao = 0, feedback = 0, result = 0, query = 0;
	GLuint primitives = 0;
	GLsizei count = [draw[@"count"] intValue];
	BOOL okay = NO;
	if (![draw[@"complete"] boolValue] || !vertex.length || !fragment.length || count <= 0 || count > 100000) return NO;
	if ([vertex containsString:@"gl_VertexID"]) return NO; /* index expansion would change its semantics */
	for (int i = 0; i < 2; i++) {
		GLint compiled = 0;
		const char *source = (i ? fragment : vertex).UTF8String;
		shaders[i] = glCreateShader(i ? GL_FRAGMENT_SHADER : GL_VERTEX_SHADER);
		glShaderSource(shaders[i], 1, &source, NULL);
		glCompileShader(shaders[i]);
		glGetShaderiv(shaders[i], GL_COMPILE_STATUS, &compiled);
		if (!compiled) goto cleanup;
	}
	program = glCreateProgram();
	glAttachShader(program, shaders[0]); glAttachShader(program, shaders[1]);
	const char *varying = "gl_Position";
	glTransformFeedbackVaryings(program, 1, &varying, GL_INTERLEAVED_ATTRIBS);
	glLinkProgram(program);
	GLint linked = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked) {
		char log[2048]; glGetProgramInfoLog(program, sizeof(log), NULL, log);
		xg_log("draw replay: %s link failed: %s", label.UTF8String, log);
		goto cleanup;
	}
	glUseProgram(program);
	for (NSDictionary *uniform in draw[@"uniforms"]) {
		GLfloat values[4] = { 0 };
		NSArray *bits = uniform[@"bits"];
		for (NSUInteger i = 0; i < bits.count && i < 4; i++) {
			uint32_t word = [bits[i] unsignedIntValue]; memcpy(&values[i], &word, 4);
		}
		GLint location = glGetUniformLocation(program, [uniform[@"name"] UTF8String]);
		if (location < 0) continue;
		if (bits.count == 1) glUniform1fv(location, 1, values); else glUniform4fv(location, 1, values);
	}
	/* Keep mixed sampler types on their original distinct units. Rasterization
 * is discarded; texture contents and pixel constants are not replayed. */
	for (int i = 0; i < 4; i++) {
		char name[16]; snprintf(name, sizeof(name), "tex%d", i);
		GLint location = glGetUniformLocation(program, name);
		if (location >= 0) glUniform1i(location, i);
	}
	glGenVertexArrays(1, &vao); glBindVertexArray(vao);
	for (NSString *key in draw[@"buffers"]) {
		NSDictionary *record = draw[@"buffers"][key];
		NSData *data = [NSData dataWithContentsOfFile:[folder stringByAppendingPathComponent:record[@"file"]]];
		if (!data || data.length != [record[@"size"] unsignedLongLongValue]) goto cleanup;
		buffers[key] = data;
	}
	elements = buffers[[draw[@"element_buffer"] stringValue]];
	NSUInteger index_offset = [draw[@"index_offset"] unsignedLongLongValue];
	GLenum index_type = [draw[@"index_type"] unsignedIntValue];
	NSUInteger index_size = index_type == GL_UNSIGNED_SHORT ? 2 : index_type == GL_UNSIGNED_INT ? 4 : 0;
	if (!index_size || index_offset + count * index_size > elements.length) goto cleanup;
	for (int i = 0; i < 16; i++) {
		NSDictionary *attribute = draw[@"attributes"][i];
		if ([attribute[@"enabled"] boolValue]) {
			NSData *data = buffers[[attribute[@"buffer"] stringValue]];
			GLenum type = [attribute[@"type"] unsignedIntValue];
			int size = [attribute[@"size"] intValue];
			NSUInteger component = type == GL_BYTE || type == GL_UNSIGNED_BYTE ? 1 :
				type == GL_SHORT || type == GL_UNSIGNED_SHORT || type == GL_HALF_FLOAT ? 2 : 4;
			NSUInteger bytes = type == GL_INT_2_10_10_10_REV || type == GL_UNSIGNED_INT_2_10_10_10_REV ? 4 : size * component;
			NSUInteger stride = [attribute[@"stride"] unsignedIntValue] ?: bytes;
			NSUInteger offset = [attribute[@"offset"] unsignedLongLongValue];
			NSMutableData *expanded = [NSMutableData dataWithLength:count * bytes];
			for (int n = 0; n < count; n++) {
				uint32_t index = 0;
				memcpy(&index, (const unsigned char *)elements.bytes + index_offset + n * index_size, index_size);
				NSUInteger start = offset + index * stride;
				if (start + bytes > data.length) goto cleanup;
				memcpy((unsigned char *)expanded.mutableBytes + n * bytes, (const unsigned char *)data.bytes + start, bytes);
			}
			GLuint handle = 0; glGenBuffers(1, &handle); [allocated addObject:@(handle)];
			glBindBuffer(GL_ARRAY_BUFFER, handle);
			glBufferData(GL_ARRAY_BUFFER, expanded.length, expanded.bytes, GL_STATIC_DRAW);
			if ([attribute[@"integer"] boolValue]) glVertexAttribIPointer(i, size, type, bytes, NULL);
			else glVertexAttribPointer(i, size, type, [attribute[@"normalized"] boolValue], bytes, NULL);
			glEnableVertexAttribArray(i);
		} else {
			GLfloat values[4];
			for (int j = 0; j < 4; j++) { uint32_t bits = [attribute[@"current_bits"][j] unsignedIntValue]; memcpy(&values[j], &bits, 4); }
			glDisableVertexAttribArray(i); glVertexAttrib4fv(i, values);
		}
	}
	glGenTransformFeedbacks(1, &feedback); glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, feedback);
	glGenBuffers(1, &result); glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, result);
	glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, count * 4 * sizeof(float), NULL, GL_STREAM_READ);
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, result);
	glGenQueries(1, &query); glEnable(GL_RASTERIZER_DISCARD);
	glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, query);
	glBeginTransformFeedback(GL_TRIANGLES);
	glDrawArrays(GL_TRIANGLES, 0, count);
	glEndTransformFeedback(); glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
	glGetQueryObjectuiv(query, GL_QUERY_RESULT, &primitives);
	if (primitives * 3 != count) goto cleanup;
	const void *bytes = glMapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, count * 4 * sizeof(float), GL_MAP_READ_BIT);
	if (bytes) {
		positions = [NSData dataWithBytes:bytes length:count * 4 * sizeof(float)];
		okay = glUnmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	}
cleanup:;
	GLenum error = glGetError();
	okay = okay && error == GL_NO_ERROR;
	if (okay) okay = [positions writeToFile:[output stringByAppendingPathComponent:[label stringByAppendingString:@"-position.bin"]] atomically:YES];
	xg_log("draw replay: %s vertices %u expected %d complete %d error 0x%x", label.UTF8String, primitives * 3, count, okay, error);
	glDisable(GL_RASTERIZER_DISCARD); glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
	glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0); glBindVertexArray(0); glBindBuffer(GL_ARRAY_BUFFER, 0); glUseProgram(0);
	for (int i = 0; i < 16; i++) glVertexAttrib4f(i, 0, 0, 0, 1);
	for (NSNumber *number in allocated) { GLuint buffer = number.unsignedIntValue; glDeleteBuffers(1, &buffer); }
	glDeleteQueries(1, &query); glDeleteBuffers(1, &result); glDeleteTransformFeedbacks(1, &feedback); glDeleteVertexArrays(1, &vao);
	if (program) glDeleteProgram(program);
	for (int i = 0; i < 2; i++) if (shaders[i]) glDeleteShader(shaders[i]);
	return okay;
}

void xg_draw_replay(void)
{
	const char *input = getenv("XG_DRAW_REPLAY"), *output = getenv("XG_DRAW_REPLAY_OUT");
	if (!input || !output) return;
	[NSFileManager.defaultManager createDirectoryAtPath:@(output) withIntermediateDirectories:YES attributes:nil error:nil];
	BOOL base = replay_draw(@(input), @(output), @"base"), equal = replay_draw(@(input), @(output), @"equal");
	xg_log("draw replay: pair complete %d", base && equal);
}
#endif
