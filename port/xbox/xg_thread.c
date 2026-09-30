/*
 * xg_thread.c: threads that run guest code.
 *
 * Guest code keeps stack addresses in 32-bit registers, so a thread running
 * it needs its stack inside guest memory. Threads the guest creates get one
 * from pthread_create; a host thread that calls into the guest later (SDL's
 * audio thread) switches to a guest stack of its own for each call
 * (xg_enter). The guest's thread pointer (its musl struct pthread) is kept
 * per thread here.
 */
#include "xg_host.h"

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#define GUARD 0x4000u
#define ENTER_STACK (512u * 1024u)

static __thread uint32_t guest_tp;
static __thread uintptr_t enter_stack_top;

uint32_t xh_host_get_tp(void) { return guest_tp; }
void xh_host_set_tp(uint32_t thread) { guest_tp = thread; }

static int on_guest_stack(void)
{
	uintptr_t sp = (uintptr_t)__builtin_frame_address(0);
	return sp - xg_base < 0x100000000ull;
}

static uintptr_t stack_allocate(uint32_t size, uint32_t *guest_low)
{
	uint32_t low = xg_map(size + GUARD, PROT_READ | PROT_WRITE);
	if (!low)
		return 0;
	mprotect(G(void *, low), GUARD, PROT_NONE);
	*guest_low = low;
	return G(uintptr_t, low + GUARD);
}

uint32_t xg_enter(uint32_t fn, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
	if (on_guest_stack())
	{
		if (!guest_tp)
			xg_call(xg_header->thread_attach, 0, 0, 0, 0, 0, 0);
		return (uint32_t)xg_call(fn, a, b, c, d, 0, 0);
	}
	if (!enter_stack_top)
	{
		uint32_t low;
		uintptr_t bottom = stack_allocate(ENTER_STACK, &low);
		if (!bottom)
			xg_fatal("cannot allocate a guest stack");
		enter_stack_top = bottom + ENTER_STACK;
	}
	if (!guest_tp)
		xg_call_on(enter_stack_top, xg_header->thread_attach, 0, 0, 0, 0);
	return (uint32_t)xg_call_on(enter_stack_top, fn, a, b, c, d);
}

struct start
{
	uint32_t function, argument;
};

static void *thread_main(void *context)
{
	struct start start = *(struct start *)context;
	free(context);
	xg_call(start.function, start.argument, 0, 0, 0, 0, 0);
	return NULL;
}

static int start_thread(uint32_t function, uint32_t argument, uint32_t stack_size)
{
	pthread_attr_t attributes;
	pthread_t thread;
	struct start *start = malloc(sizeof(*start));
	uint32_t low, size = (stack_size + XG_PAGE - 1) & ~(XG_PAGE - 1);
	uintptr_t bottom;
	int error;
	if (size < 256u * 1024u)
		size = 256u * 1024u;
	bottom = stack_allocate(size, &low);
	if (!start || !bottom)
	{
		free(start);
		return EAGAIN;
	}
	start->function = function;
	start->argument = argument;
	pthread_attr_init(&attributes);
	pthread_attr_setstack(&attributes, (void *)bottom, size);
	pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
	error = pthread_create(&thread, &attributes, thread_main, start);
	pthread_attr_destroy(&attributes);
	/* stacks are not reclaimed: the game makes a handful of long-lived threads */
	return error;
}

int xh_host_thread_create(uint32_t thread, uint32_t stack_size)
{
	return start_thread(xg_header->thread_start, thread, stack_size);
}

int xg_start_game(uint32_t boot)
{
	return start_thread(xg_header->start, boot, 16u * 1024u * 1024u);
}

uint32_t xg_make_boot(const char **environment, int count, int argc, char **argv)
{
	uint32_t boot = xg_map(0x10000, PROT_READ | PROT_WRITE);
	uint32_t argv_list = boot + 32, environment_list = argv_list + 4 * 16, strings = environment_list + 4 * 64;
	int index;
	if (!boot)
		return 0;
	for (index = 0; index < argc && index < 15; index++)
	{
		strcpy(G(char *, strings), argv[index]);
		G(uint32_t *, argv_list)[index] = strings;
		strings += (uint32_t)strlen(argv[index]) + 1;
	}
	G(uint32_t *, argv_list)[index] = 0;
	for (index = 0; index < count && index < 63; index++)
	{
		strcpy(G(char *, strings), environment[index]);
		G(uint32_t *, environment_list)[index] = strings;
		strings += (uint32_t)strlen(environment[index]) + 1;
	}
	G(uint32_t *, environment_list)[index] = 0;
	G(uint32_t *, boot)[0] = (uint32_t)(argc < 15 ? argc : 15);
	G(uint32_t *, boot)[1] = argv_list;
	G(uint32_t *, boot)[2] = environment_list;
	G(uint32_t *, boot)[3] = XG_PAGE;
	return boot;
}
