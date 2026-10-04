/* Asset-free Simulator ANGLE/Metal proof. Not Halo rendering acceptance. */
#import <Foundation/Foundation.h>
#include <TargetConditionals.h>
#if !TARGET_OS_SIMULATOR
#error This probe is Simulator-only
#endif
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <stdio.h>

static GLuint shader(GLenum kind, const char *source)
{
    GLuint value = glCreateShader(kind);
    glShaderSource(value, 1, &source, NULL); glCompileShader(value);
    GLint okay = 0; glGetShaderiv(value, GL_COMPILE_STATUS, &okay);
    if (!okay) { char log[1024]; glGetShaderInfoLog(value, sizeof(log), NULL, log); fprintf(stderr, "%s\n", log); return 0; }
    return value;
}

int main(void)
{
    @autoreleasepool {
        const char *enabled[] = {"hasTextureSwizzle", NULL}, *empty[] = {NULL};
        const EGLAttrib display_attributes[] = {EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE,
            EGL_FEATURE_OVERRIDES_ENABLED_ANGLE, (EGLAttrib)(getenv("HALOPAD_ANGLE_NATIVE_SWIZZLE") ? enabled : empty), EGL_NONE};
        EGLDisplay display = eglGetPlatformDisplay(EGL_PLATFORM_ANGLE_ANGLE, NULL, display_attributes);
        EGLint major = 0, minor = 0, count = 0;
        if (!eglInitialize(display, &major, &minor)) { fprintf(stderr, "EGL initialization failed: 0x%x\n", eglGetError()); return 1; }
        const EGLint config_attributes[] = {EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8, EGL_DEPTH_SIZE, 24, EGL_STENCIL_SIZE, 8, EGL_NONE};
        EGLConfig config;
        if (!eglChooseConfig(display, config_attributes, &config, 1, &count) || count != 1) return 2;
        const EGLint surface_attributes[] = {EGL_WIDTH, 64, EGL_HEIGHT, 64, EGL_NONE};
        const EGLint context_attributes[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
        EGLSurface surface = eglCreatePbufferSurface(display, config, surface_attributes);
        EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attributes);
        if (!eglMakeCurrent(display, surface, surface, context)) { fprintf(stderr, "EGL context failed: 0x%x\n", eglGetError()); return 3; }
        printf("renderer: %s; version: %s\n", glGetString(GL_RENDERER), glGetString(GL_VERSION));
        const char *sources[] = {
            "#version 300 es\nprecision highp float;layout(location=0)in vec4 p;invariant gl_Position;void main(){gl_Position=p;}\n",
            "#version 300 es\nprecision highp float;layout(location=0)in vec4 p;out vec2 uv;invariant gl_Position;void main(){gl_Position=p;uv=p.xy;}\n",
            "#version 300 es\nprecision highp float;uniform vec4 color;out vec4 c;void main(){c=color;}\n",
            "#version 300 es\nprecision highp float;in vec2 uv;uniform vec4 color;out vec4 c;void main(){c=color+vec4(uv.x*0.001,0,0,0);}\n"};
        GLuint shaders[4], programs[2];
        for (int i = 0; i < 4; i++) if (!(shaders[i] = shader(i < 2 ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER, sources[i]))) return 4;
        for (int i = 0; i < 2; i++) {
            programs[i] = glCreateProgram(); glAttachShader(programs[i], shaders[i]); glAttachShader(programs[i], shaders[i + 2]);
            glLinkProgram(programs[i]); GLint linked = 0; glGetProgramiv(programs[i], GL_LINK_STATUS, &linked); if (!linked) return 5;
        }
        const GLfloat vertices[] = {-0.8f,-0.8f,0.2f,1, 0.8f,-0.8f,0.7f,1, 0,0.8f,0.4f,1};
        GLuint vao, buffer; glGenVertexArrays(1, &vao); glBindVertexArray(vao);
        glGenBuffers(1, &buffer); glBindBuffer(GL_ARRAY_BUFFER, buffer); glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL); glEnableVertexAttribArray(0);
        glViewport(0,0,64,64); glEnable(GL_DEPTH_TEST);
        unsigned char base[64*64*4], result[64*64*4];
        int failures = 0;
        for (int control = 0; control < 2; control++) {
            glDepthMask(GL_TRUE); glClearColor(0,0,0,1); glClearDepthf(1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDepthFunc(GL_LESS); glUseProgram(programs[0]); glUniform4f(glGetUniformLocation(programs[0], "color"), 1,0,0,1);
            glDrawArrays(GL_TRIANGLES,0,3); glReadPixels(0,0,64,64,GL_RGBA,GL_UNSIGNED_BYTE,base);
            glDepthMask(GL_FALSE); glDepthFunc(GL_EQUAL); glUseProgram(programs[control]);
            glUniform4f(glGetUniformLocation(programs[control], "color"), 0,0,1,1);
            glDrawArrays(GL_TRIANGLES,0,3); glReadPixels(0,0,64,64,GL_RGBA,GL_UNSIGNED_BYTE,result);
            int covered = 0, failed = 0; for (int i = 0; i < 64*64; i++) if (base[i*4] > 200) { covered++; failed += result[i*4+2] < 200; }
            GLenum error = glGetError(); printf("equal control %d: covered %d failed %d error 0x%x\n",control,covered,failed,error);
            failures += !covered || failed || error;
        }
        /* A texture's sampling swizzle must not affect framebuffer blits.
         * The guest retains BGRA sampling swizzles on some render targets. */
        GLuint texture, framebuffer;
        glGenTextures(1, &texture); glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,64,64,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
        glGenFramebuffers(1, &framebuffer); glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texture,0);
        glDisable(GL_DEPTH_TEST); glClearColor(0.8f,0.1f,0.05f,1); glClear(GL_COLOR_BUFFER_BIT);
        for (int swizzled = 0; swizzled < 2; swizzled++) {
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_SWIZZLE_R,swizzled ? GL_BLUE : GL_RED);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_SWIZZLE_B,swizzled ? GL_RED : GL_BLUE);
            glBindFramebuffer(GL_READ_FRAMEBUFFER,framebuffer); glBindFramebuffer(GL_DRAW_FRAMEBUFFER,0);
            glBlitFramebuffer(0,0,64,64,0,0,64,64,GL_COLOR_BUFFER_BIT,GL_NEAREST);
            glBindFramebuffer(GL_READ_FRAMEBUFFER,0); unsigned char pixel[4] = {0};
            glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
            GLenum error = glGetError();
            printf("blit swizzle %d: rgba %d %d %d %d error 0x%x\n",swizzled,pixel[0],pixel[1],pixel[2],pixel[3],error);
            failures += pixel[0] < 200 || pixel[2] > 20 || error;
        }
        GLuint sample_shader = shader(GL_FRAGMENT_SHADER,
            "#version 300 es\nprecision highp float;uniform sampler2D t;out vec4 c;void main(){c=texture(t,vec2(0.5));}\n");
        GLuint sample_program = glCreateProgram(); glAttachShader(sample_program,shaders[0]);
        glAttachShader(sample_program,sample_shader); glLinkProgram(sample_program);
        GLint sample_linked = 0; glGetProgramiv(sample_program,GL_LINK_STATUS,&sample_linked); if (!sample_linked) return 5;
        glUseProgram(sample_program); glUniform1i(glGetUniformLocation(sample_program,"t"),0);
        glBindFramebuffer(GL_FRAMEBUFFER,0); glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,texture);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);
        for (int swizzled = 0; swizzled < 2; swizzled++) {
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_SWIZZLE_R,swizzled ? GL_BLUE : GL_RED);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_SWIZZLE_B,swizzled ? GL_RED : GL_BLUE);
            glDrawArrays(GL_TRIANGLES,0,3); unsigned char pixel[4] = {0};
            glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
            GLenum error = glGetError();
            printf("sample swizzle %d: rgba %d %d %d %d error 0x%x\n",swizzled,pixel[0],pixel[1],pixel[2],pixel[3],error);
            failures += pixel[swizzled ? 2 : 0] < 200 || pixel[swizzled ? 0 : 2] > 20 || error;
        }
        eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
        eglDestroyContext(display,context); eglDestroySurface(display,surface); eglTerminate(display);
        return failures ? 6 : 0;
    }
}
