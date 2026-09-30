/*
 * xg_ios.m: the Xbox engine's platform services on iOS and iPadOS, without
 * SDL: the host half of upstream's guest_sdl.c on UIKit, OpenGL ES (Apple's
 * OpenGL ES 3.0), the GameController framework and Core Audio.
 *
 * The game runs on a thread of its own (xg_ios_start); the view is made on
 * the main thread by the app. iOS has no default framebuffer, so the game's
 * framebuffer 0 is a framebuffer with the view's layer as its colour
 * buffer (xg_gl_framebuffer).
 */
#import <AudioToolbox/AudioToolbox.h>
#import <AVFoundation/AVFoundation.h>
#import <GameController/GameController.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES3/gl.h>
#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_audio.h>
#include <dlfcn.h>
#include <mach/mach_time.h>
#include <pthread.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "xg_host.h"
#include "xg_ios.h"

void xg_gl_load(void);
void xg_gl_frame_dump(int width, int height);

struct xg_paths xg_paths;

/* ---------- the view */

@interface XGGameView : UIView
@end

@implementation XGGameView
+ (Class)layerClass { return [CAEAGLLayer class]; }
@end

static XGGameView *game_view;
static CAEAGLLayer *game_layer;
static EAGLContext *context;
static GLuint drawable_framebuffer, drawable_color;
static GLint drawable_width, drawable_height;
static CGSize layer_pixels;
static pthread_mutex_t layer_lock = PTHREAD_MUTEX_INITIALIZER;

UIView *xg_ios_make_view(CGRect frame)
{
	game_view = [[XGGameView alloc] initWithFrame:frame];
	game_view.contentScaleFactor = UIScreen.mainScreen.nativeScale;
	game_view.multipleTouchEnabled = YES;
	game_view.backgroundColor = UIColor.blackColor;
	game_layer = (CAEAGLLayer *)game_view.layer;
	game_layer.opaque = YES;
	game_layer.drawableProperties = @{ kEAGLDrawablePropertyRetainedBacking: @NO,
		kEAGLDrawablePropertyColorFormat: kEAGLColorFormatRGBA8 };
	xg_ios_view_resized();
	return game_view;
}

/* the main thread: the view's size in pixels, for the game thread */
void xg_ios_view_resized(void)
{
	CGSize size = game_view.bounds.size;
	CGFloat scale = game_view.contentScaleFactor;
	pthread_mutex_lock(&layer_lock);
	layer_pixels = CGSizeMake(size.width * scale, size.height * scale);
	pthread_mutex_unlock(&layer_lock);
}

static void drawable_update(void)
{
	CGSize pixels;
	pthread_mutex_lock(&layer_lock);
	pixels = layer_pixels;
	pthread_mutex_unlock(&layer_lock);
	if (drawable_color && (GLint)pixels.width == drawable_width && (GLint)pixels.height == drawable_height)
		return;
	if (!drawable_framebuffer)
	{
		glGenFramebuffers(1, &drawable_framebuffer);
		glGenRenderbuffers(1, &drawable_color);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, drawable_framebuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, drawable_color);
	[context renderbufferStorage:GL_RENDERBUFFER fromDrawable:game_layer];
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, drawable_color);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &drawable_width);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &drawable_height);
	xg_log("drawable %dx%d", drawable_width, drawable_height);
}

void *xg_gl_proc(const char *name) { return dlsym(RTLD_DEFAULT, name); }
GLuint xg_gl_framebuffer(GLuint framebuffer) { return framebuffer ? framebuffer : drawable_framebuffer; }

/* ---------- SDL services */

static char last_error[256];
static uint64_t start_ticks;

static void copy_out(uint32_t buffer, uint32_t size, const char *text)
{
	if (!size)
		return;
	strlcpy(G(char *, buffer), text ? text : "", size);
}

int xh_host_sdl_init(uint32_t flags) { (void)flags; return 1; }
int xh_host_sdl_set_hint(uint32_t name, uint32_t value) { (void)name; (void)value; return 1; }
void xh_host_sdl_get_error(uint32_t buffer, uint32_t size) { copy_out(buffer, size, last_error); }

long long xh_host_sdl_ticks(void)
{
	static mach_timebase_info_data_t timebase;
	if (!timebase.denom)
		mach_timebase_info(&timebase);
	return (long long)((mach_absolute_time() - start_ticks) * timebase.numer / timebase.denom / 1000000ull);
}

long long xh_host_sdl_thread_id(void)
{
	uint64_t id;
	pthread_threadid_np(NULL, &id);
	return (long long)id;
}

uint32_t xh_host_sdl_create_window(uint32_t title, int width, int height, long long flags)
{
	(void)title; (void)width; (void)height; (void)flags;
	return game_view ? 1 : 0;
}

void xh_host_sdl_window_size_in_pixels(uint32_t window, uint32_t width, uint32_t height)
{
	(void)window;
	if (width) *G(int *, width) = drawable_width;
	if (height) *G(int *, height) = drawable_height;
}

int xh_host_sdl_set_relative_mouse(uint32_t window, int enabled) { (void)window; (void)enabled; return 1; }
int xh_host_sdl_gl_set_attribute(int attribute, int value) { (void)attribute; (void)value; return 1; }

uint32_t xh_host_sdl_gl_create_context(uint32_t window)
{
	(void)window;
	if (!context)
	{
		context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
		if (!context)
		{
			strlcpy(last_error, "OpenGL ES 3 is not available", sizeof(last_error));
			return 0;
		}
		[EAGLContext setCurrentContext:context];
		drawable_update();
		xg_gl_load();
	}
	return 2;
}

int xh_host_sdl_gl_make_current(uint32_t window, uint32_t handle)
{
	(void)window;
	return [EAGLContext setCurrentContext:handle ? context : nil];
}

int xh_host_sdl_gl_set_swap_interval(int interval) { (void)interval; return 1; }

int xh_host_sdl_gl_swap_window(uint32_t window)
{
	static int frames;
	(void)window;
	if (frames < 3 || frames == 120)
	{
		GLint read = 0, draw = 0;
		unsigned char pixel[4] = { 0 };
		glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read);
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, drawable_framebuffer);
		glReadPixels(drawable_width / 2, drawable_height / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
		xg_log("frame %d: error 0x%x, status 0x%x, read %d draw %d (drawable %u), centre %d %d %d", frames,
			glGetError(), glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER), read, draw, drawable_framebuffer,
			pixel[0], pixel[1], pixel[2]);
	}
	frames++;
	glBindFramebuffer(GL_READ_FRAMEBUFFER, drawable_framebuffer);
	xg_gl_frame_dump(drawable_width, drawable_height);
	glBindRenderbuffer(GL_RENDERBUFFER, drawable_color);
	[context presentRenderbuffer:GL_RENDERBUFFER];
	drawable_update();
	return 1;
}

int xh_host_sdl_set_clipboard_text(uint32_t text) { (void)text; return 1; }
void xh_host_sdl_get_clipboard_text(uint32_t buffer, uint32_t size) { copy_out(buffer, size, ""); }

int xh_host_sdl_show_toast(uint32_t message, int duration, int gravity, int x, int y)
{
	(void)duration; (void)gravity; (void)x; (void)y;
	xg_log("%s", G(const char *, message));
	return 1;
}

int xh_host_sdl_show_simple_message_box(uint32_t flags, uint32_t title, uint32_t message)
{
	(void)flags;
	xg_log("message: %s: %s", G(const char *, title), G(const char *, message));
	return 1;
}

/* ---------- gamepads: controllers get small ids, which are also their handles */

#define PADS 8
static __strong GCController *pads[PADS + 1];
static uint8_t pad_announced[PADS + 1];
static pthread_mutex_t pad_lock = PTHREAD_MUTEX_INITIALIZER;

static void pads_refresh(void)
{
	NSArray<GCController *> *controllers = GCController.controllers;
	int index;
	pthread_mutex_lock(&pad_lock);
	for (index = 1; index <= PADS; index++)
		if (pads[index] && ![controllers containsObject:pads[index]])
		{
			pads[index] = nil;
			pad_announced[index] = 0;
		}
	for (GCController *controller in controllers)
	{
		int free_slot = 0, known = 0;
		if (!controller.extendedGamepad)
			continue;
		for (index = 1; index <= PADS; index++)
		{
			if (pads[index] == controller) known = 1;
			if (!pads[index] && !free_slot) free_slot = index;
		}
		if (!known && free_slot)
			pads[free_slot] = controller;
	}
	pthread_mutex_unlock(&pad_lock);
}

int xh_host_sdl_get_gamepads(uint32_t ids, int capacity)
{
	int count = 0, index;
	pads_refresh();
	for (index = 1; index <= PADS && count < capacity; index++)
		if (pads[index])
		{
			G(uint32_t *, ids)[count++] = (uint32_t)index;
			pad_announced[index] = 1;
		}
	return count;
}

uint32_t xh_host_sdl_open_gamepad(uint32_t id) { return id <= PADS && pads[id] ? id : 0; }
uint32_t xh_host_sdl_gamepad_from_id(uint32_t id) { return xh_host_sdl_open_gamepad(id); }

static int16_t axis_value(float value)
{
	float scaled = value * 32767.0f;
	return (int16_t)(scaled < -32768.0f ? -32768.0f : scaled > 32767.0f ? 32767.0f : scaled);
}

int xh_host_sdl_gamepad_axis(uint32_t pad, int axis)
{
	GCExtendedGamepad *g = pad <= PADS ? pads[pad].extendedGamepad : nil;
	if (!g)
		return 0;
	switch (axis)
	{
	case SDL_GAMEPAD_AXIS_LEFTX: return axis_value(g.leftThumbstick.xAxis.value);
	case SDL_GAMEPAD_AXIS_LEFTY: return axis_value(-g.leftThumbstick.yAxis.value);
	case SDL_GAMEPAD_AXIS_RIGHTX: return axis_value(g.rightThumbstick.xAxis.value);
	case SDL_GAMEPAD_AXIS_RIGHTY: return axis_value(-g.rightThumbstick.yAxis.value);
	case SDL_GAMEPAD_AXIS_LEFT_TRIGGER: return axis_value(g.leftTrigger.value);
	case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER: return axis_value(g.rightTrigger.value);
	default: return 0;
	}
}

int xh_host_sdl_gamepad_button(uint32_t pad, int button)
{
	GCExtendedGamepad *g = pad <= PADS ? pads[pad].extendedGamepad : nil;
	if (!g)
		return 0;
	switch (button)
	{
	case SDL_GAMEPAD_BUTTON_SOUTH: return g.buttonA.pressed;
	case SDL_GAMEPAD_BUTTON_EAST: return g.buttonB.pressed;
	case SDL_GAMEPAD_BUTTON_WEST: return g.buttonX.pressed;
	case SDL_GAMEPAD_BUTTON_NORTH: return g.buttonY.pressed;
	case SDL_GAMEPAD_BUTTON_BACK: return g.buttonOptions.pressed;
	case SDL_GAMEPAD_BUTTON_START: return g.buttonMenu.pressed;
	case SDL_GAMEPAD_BUTTON_LEFT_STICK: return g.leftThumbstickButton.pressed;
	case SDL_GAMEPAD_BUTTON_RIGHT_STICK: return g.rightThumbstickButton.pressed;
	case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: return g.leftShoulder.pressed;
	case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return g.rightShoulder.pressed;
	case SDL_GAMEPAD_BUTTON_DPAD_UP: return g.dpad.up.pressed;
	case SDL_GAMEPAD_BUTTON_DPAD_DOWN: return g.dpad.down.pressed;
	case SDL_GAMEPAD_BUTTON_DPAD_LEFT: return g.dpad.left.pressed;
	case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: return g.dpad.right.pressed;
	default: return 0;
	}
}

int xh_host_sdl_gamepad_type(uint32_t pad) { (void)pad; return SDL_GAMEPAD_TYPE_XBOXONE; }

int xh_host_sdl_rumble_gamepad(uint32_t pad, uint32_t low, uint32_t high, uint32_t milliseconds)
{
	(void)pad; (void)low; (void)high; (void)milliseconds;
	return 0;
}

/* ---------- events: controllers that appeared since the last poll */

int xh_host_sdl_poll_event(uint32_t event)
{
	SDL_Event *out = G(SDL_Event *, event);
	int index;
	pads_refresh();
	for (index = 1; index <= PADS; index++)
		if (pads[index] && !pad_announced[index])
		{
			pad_announced[index] = 1;
			memset(out, 0, sizeof(*out));
			out->gdevice.type = SDL_EVENT_GAMEPAD_ADDED;
			out->gdevice.timestamp = (Uint64)xh_host_sdl_ticks() * 1000000ull;
			out->gdevice.which = (SDL_JoystickID)index;
			return 1;
		}
	return 0;
}

/* ---------- audio: a Remote I/O unit pulls from the guest's callback */

static AudioComponentInstance audio_unit;
static uint32_t audio_callback, audio_userdata;
static uint8_t *audio_buffer;
static uint32_t audio_length, audio_capacity, audio_frame_bytes;

static OSStatus audio_render(void *reference, AudioUnitRenderActionFlags *flags, const AudioTimeStamp *time,
	UInt32 bus, UInt32 frames, AudioBufferList *io)
{
	uint32_t needed = frames * audio_frame_bytes, copied;
	(void)reference; (void)flags; (void)time; (void)bus;
	if (audio_length < needed && audio_callback)
		xg_enter(audio_callback, audio_userdata, 1, needed - audio_length, needed);
	copied = audio_length < needed ? audio_length : needed;
	memcpy(io->mBuffers[0].mData, audio_buffer, copied);
	memset((uint8_t *)io->mBuffers[0].mData + copied, 0, needed - copied);
	memmove(audio_buffer, audio_buffer + copied, audio_length - copied);
	audio_length -= copied;
	return noErr;
}

uint32_t xh_host_sdl_open_audio_stream(uint32_t device, uint32_t spec_address, uint32_t callback, uint32_t userdata)
{
	const SDL_AudioSpec *spec = G(const SDL_AudioSpec *, spec_address);
	AudioComponentDescription description = { kAudioUnitType_Output, kAudioUnitSubType_RemoteIO,
		kAudioUnitManufacturer_Apple, 0, 0 };
	AudioStreamBasicDescription format;
	AURenderCallbackStruct render = { audio_render, NULL };
	AudioComponent component;
	(void)device;
	if (spec->format != SDL_AUDIO_F32)
	{
		xg_log("audio format 0x%x is not supported", spec->format);
		return 0;
	}
	[[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryAmbient error:nil];
	[[AVAudioSession sharedInstance] setActive:YES error:nil];
	audio_callback = callback;
	audio_userdata = userdata;
	audio_frame_bytes = 4u * (uint32_t)spec->channels;
	audio_capacity = 1u << 20;
	audio_buffer = malloc(audio_capacity);
	memset(&format, 0, sizeof(format));
	format.mSampleRate = spec->freq;
	format.mFormatID = kAudioFormatLinearPCM;
	format.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
	format.mBytesPerPacket = format.mBytesPerFrame = audio_frame_bytes;
	format.mFramesPerPacket = 1;
	format.mChannelsPerFrame = (UInt32)spec->channels;
	format.mBitsPerChannel = 32;
	component = AudioComponentFindNext(NULL, &description);
	if (!component || AudioComponentInstanceNew(component, &audio_unit) ||
		AudioUnitSetProperty(audio_unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &format, sizeof(format)) ||
		AudioUnitSetProperty(audio_unit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &render, sizeof(render)) ||
		AudioUnitInitialize(audio_unit))
	{
		xg_log("cannot open audio output");
		return 0;
	}
	xg_log("audio %d Hz, %d channels", spec->freq, spec->channels);
	return 1;
}

int xh_host_sdl_put_audio_stream_data(uint32_t stream, uint32_t data, int length)
{
	(void)stream;
	if (length <= 0)
		return 1;
	if (audio_length + (uint32_t)length > audio_capacity)
		length = (int)(audio_capacity - audio_length);
	memcpy(audio_buffer + audio_length, G(const void *, data), (size_t)length);
	audio_length += (uint32_t)length;
	return 1;
}

int xh_host_sdl_resume_audio_stream_device(uint32_t stream)
{
	(void)stream;
	return audio_unit && AudioOutputUnitStart(audio_unit) == noErr;
}

/* ---------- start-up */

int xg_ios_start(const char *image_path, const char *data_root, const char *save_root)
{
	NSData *image;
	char buffers[5][1100];
	const char *environment[6];
	CGSize screen = UIScreen.mainScreen.bounds.size;
	CGFloat longer = MAX(screen.width, screen.height), shorter = MIN(screen.width, screen.height);
	uint32_t boot;
	start_ticks = mach_absolute_time();
	strlcpy(xg_paths.data_root, data_root, sizeof(xg_paths.data_root));
	strlcpy(xg_paths.save_root, save_root, sizeof(xg_paths.save_root));
	mkdir(save_root, 0755);
	xg_install_signal_handlers();
	if (xg_memory_initialize())
	{
		xg_log("cannot reserve guest memory (the app needs the extended virtual addressing entitlement)");
		return -1;
	}
	image = [NSData dataWithContentsOfFile:@(image_path)];
	if (!image || xg_load_image(image.bytes, image.length))
	{
		xg_log("cannot load the game image %s", image_path);
		return -1;
	}
	snprintf(buffers[0], sizeof(buffers[0]), "HOME=%s", save_root);
	snprintf(buffers[1], sizeof(buffers[1]), "HALO_DATA_ROOT=%s", data_root);
	snprintf(buffers[2], sizeof(buffers[2]), "HALO_SAVE_ROOT=%s", save_root);
	/* the game draws 480 lines at the screen's shape (upstream's d3d8_gl.c) */
	snprintf(buffers[3], sizeof(buffers[3]), "HALO_DISPLAY_WIDTH=%d", (int)(480.0 * longer / shorter) & ~1);
	{
		NSInteger offset = -NSTimeZone.localTimeZone.secondsFromGMT;
		snprintf(buffers[4], sizeof(buffers[4]), "TZ=<L>%s%ld:%02ld", offset < 0 ? "-" : "",
			labs((long)offset) / 3600, (labs((long)offset) / 60) % 60);
	}
	for (int index = 0; index < 5; index++)
		environment[index] = buffers[index];
	boot = xg_make_boot(environment, 5, 1, (char *[]){ "halo" });
	if (!boot || xg_start_game(boot))
	{
		xg_log("cannot start the game thread");
		return -1;
	}
	xg_log("started: data %s, saves %s", data_root, save_root);
	return 0;
}
