/* Asset-free real-GPU checks of the private counted ANGLE candidate. */
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <initializer_list>
#include <Foundation/Foundation.h>
extern "C" int halopad_angle_query_samples(unsigned, unsigned *);
extern "C" int halopad_angle_buffer_read_write_enabled(void);
#define REQUIRE(x) do { if (!(x)) { fprintf(stderr, "FAIL line %d: %s (GL 0x%x EGL 0x%x)\n", __LINE__, #x, glGetError(), eglGetError()); exit(1); } } while (0)
static unsigned checks;

static GLuint shader(GLenum type, const char *source)
{
    GLuint s = glCreateShader(type); glShaderSource(s, 1, &source, NULL); glCompileShader(s);
    GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok); REQUIRE(ok); return s;
}

static GLuint target(int size)
{
    GLuint f, t, d;
    glGenFramebuffers(1, &f); glBindFramebuffer(GL_FRAMEBUFFER, f);
    glGenTextures(1, &t); glBindTexture(GL_TEXTURE_2D, t);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, size, size);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t, 0);
    glGenRenderbuffers(1, &d); glBindRenderbuffer(GL_RENDERBUFFER, d);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size, size);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, d);
    REQUIRE(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    return f;
}

static void expect(GLuint q, unsigned count, const char *label)
{
    GLuint boolean = 9, samples = 9;
    glGetQueryObjectuiv(q, GL_QUERY_RESULT, &boolean);
    REQUIRE(boolean == (count ? GL_TRUE : GL_FALSE));
    REQUIRE(halopad_angle_query_samples(q, &samples));
    fprintf(stderr, "%s: count %u expected %u GLES boolean %u\n", label, samples, count, boolean);
    REQUIRE(samples == count); REQUIRE(glGetError() == GL_NO_ERROR); checks++;
}

static void draw(void) { glDrawArrays(GL_TRIANGLES, 0, 3); }

int main(int argc, char **argv)
{
    @autoreleasepool {
        unsigned ignored;
        REQUIRE(!halopad_angle_query_samples(1, &ignored));
        const char *feature[] = {"allowBufferReadWrite", NULL};
        REQUIRE(argc == 2 && (!strcmp(argv[1], "on") || !strcmp(argv[1], "off")));
        const EGLAttrib attrs[] = {EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE,
            !strcmp(argv[1], "on") ? EGL_FEATURE_OVERRIDES_ENABLED_ANGLE : EGL_FEATURE_OVERRIDES_DISABLED_ANGLE,
            reinterpret_cast<EGLAttrib>(feature), EGL_NONE};
        EGLDisplay display = eglGetPlatformDisplay(EGL_PLATFORM_ANGLE_ANGLE, NULL, attrs);
        REQUIRE(eglInitialize(display, NULL, NULL));
        EGLConfig config; EGLint count;
        const EGLint cfg[] = {EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_NONE};
        REQUIRE(eglChooseConfig(display, cfg, &config, 1, &count) && count == 1);
        const EGLint ctx[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
        const EGLint pb[] = {EGL_WIDTH, 16, EGL_HEIGHT, 16, EGL_NONE};
        EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctx);
        EGLSurface surface = eglCreatePbufferSurface(display, config, pb);
        REQUIRE(context != EGL_NO_CONTEXT && surface != EGL_NO_SURFACE);
        REQUIRE(eglMakeCurrent(display, surface, surface, context));
        REQUIRE(halopad_angle_buffer_read_write_enabled() == (!strcmp(argv[1], "on") ? 1 : 0));
        fprintf(stderr, "renderer: %s; allowBufferReadWrite forced %s\n", glGetString(GL_RENDERER), argv[1]);
        GLuint p = glCreateProgram();
        glAttachShader(p, shader(GL_VERTEX_SHADER, "#version 300 es\nvoid main(){vec2 p=gl_VertexID==0?vec2(-1,-1):(gl_VertexID==1?vec2(3,-1):vec2(-1,3));gl_Position=vec4(p,0,1);}"));
        glAttachShader(p, shader(GL_FRAGMENT_SHADER, "#version 300 es\nprecision highp float;out vec4 c;void main(){c=vec4(1);}"));
        glLinkProgram(p); GLint linked; glGetProgramiv(p, GL_LINK_STATUS, &linked); REQUIRE(linked); glUseProgram(p);
        GLuint q; glGenQueries(1, &q);
        REQUIRE(!halopad_angle_query_samples(0, &ignored));
        REQUIRE(!halopad_angle_query_samples(q, NULL));
        for (int size : {64, 128, 512}) {
            GLuint a = target(size), b = target(size);
            glBindFramebuffer(GL_FRAMEBUFFER, a); glViewport(0, 0, size, size);
            glDisable(GL_SCISSOR_TEST); glDisable(GL_DEPTH_TEST);
            glBeginQuery(GL_ANY_SAMPLES_PASSED, q);
            REQUIRE(!halopad_angle_query_samples(q, &ignored));
            glEndQuery(GL_ANY_SAMPLES_PASSED); expect(q, 0, "empty/reuse");
            glBeginQuery(GL_ANY_SAMPLES_PASSED, q); draw(); glEndQuery(GL_ANY_SAMPLES_PASSED);
            expect(q, size * size, "full"); expect(q, size * size, "repeated read");
            glEnable(GL_SCISSOR_TEST); glScissor(0, 0, size / 2, size);
            glBeginQuery(GL_ANY_SAMPLES_PASSED, q); draw(); glEndQuery(GL_ANY_SAMPLES_PASSED);
            expect(q, size * size / 2, "half scissor");
            glDisable(GL_SCISSOR_TEST); glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LESS);
            glClearDepthf(0); glClear(GL_DEPTH_BUFFER_BIT);
            glBeginQuery(GL_ANY_SAMPLES_PASSED, q); draw(); glEndQuery(GL_ANY_SAMPLES_PASSED);
            expect(q, 0, "fully depth hidden");
            glClearDepthf(1); glClear(GL_DEPTH_BUFFER_BIT);
            glEnable(GL_SCISSOR_TEST); glScissor(0, 0, size / 2, size);
            glClearDepthf(0); glClear(GL_DEPTH_BUFFER_BIT); glDisable(GL_SCISSOR_TEST);
            glBeginQuery(GL_ANY_SAMPLES_PASSED, q); draw(); glEndQuery(GL_ANY_SAMPLES_PASSED);
            expect(q, size * size / 2, "half depth hidden");
            glDisable(GL_DEPTH_TEST);
            glBeginQuery(GL_ANY_SAMPLES_PASSED, q); draw(); glFlush();
            glBindFramebuffer(GL_FRAMEBUFFER, b);
            glEnable(GL_SCISSOR_TEST); glScissor(0, 0, size / 2, size); draw();
            glEndQuery(GL_ANY_SAMPLES_PASSED);
            expect(q, size * size * 3 / 2, "two passes with flush");
            glDisable(GL_SCISSOR_TEST);
            GLuint conservative; glGenQueries(1, &conservative);
            glBeginQuery(GL_ANY_SAMPLES_PASSED_CONSERVATIVE, conservative); draw();
            glEndQuery(GL_ANY_SAMPLES_PASSED_CONSERVATIVE);
            expect(conservative, size * size, "conservative query");
            glDeleteQueries(1, &conservative);
            glDeleteFramebuffers(1, &a); glDeleteFramebuffers(1, &b);
        }
        target(640); glDisable(GL_DEPTH_TEST); glDisable(GL_SCISSOR_TEST);
        glViewport(-42, 203, 58, 58);
        glBeginQuery(GL_ANY_SAMPLES_PASSED, q); draw(); glEndQuery(GL_ANY_SAMPLES_PASSED);
        expect(q, 928, "measured sun-edge rectangle");
        glViewport(-66, 189, 59, 59);
        glBeginQuery(GL_ANY_SAMPLES_PASSED, q); draw(); glEndQuery(GL_ANY_SAMPLES_PASSED);
        expect(q, 0, "measured outside rectangle");
        GLuint second; glGenQueries(1, &second); glViewport(0, 0, 64, 64);
        glBeginQuery(GL_ANY_SAMPLES_PASSED, q); draw(); glEndQuery(GL_ANY_SAMPLES_PASSED);
        glViewport(0, 0, 32, 64);
        glBeginQuery(GL_ANY_SAMPLES_PASSED, second); draw(); glEndQuery(GL_ANY_SAMPLES_PASSED);
        expect(second, 2048, "independent second query read first");
        expect(q, 4096, "independent first query retained");
        fprintf(stderr, "PASS: %u counted visibility cases, standard GLES remains boolean\n", checks);
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroySurface(display, surface); eglDestroyContext(display, context); eglTerminate(display);
        return 0;
    }
}
