/*
 * xg_fault_frames.h: frame-record walking for the fault reporter that never
 * faults itself.
 *
 * Translated guest code keeps guest addresses in its registers, so a frame
 * pointer inside guest code is a 32-bit guest address (the host address is
 * xg_base + fp), while host code holds full host addresses. Records are read
 * with a kernel copy, so an unmapped or corrupt chain ends the walk instead
 * of raising another signal inside the handler.
 */
#ifndef XG_FAULT_FRAMES_H
#define XG_FAULT_FRAMES_H

#include <mach/mach.h>
#include <stdint.h>

typedef int (*xg_frame_reader)(uint64_t address, uint64_t record[2]);

static inline uint64_t xg_frame_host_address(uint64_t fp, uint64_t base)
{
	return fp < 0x100000000ull ? base + fp : fp;
}

static inline int xg_read_frame_record(uint64_t address, uint64_t record[2])
{
	vm_size_t copied = 0;
	return vm_read_overwrite(mach_task_self(), (vm_address_t)address, 16,
		(vm_address_t)record, &copied) == KERN_SUCCESS && copied == 16;
}

/* Return addresses of up to maximum records, outermost last. Stops at a null,
 * misaligned, unreadable or non-ascending frame pointer. */
static inline int xg_walk_frames(uint64_t fp, uint64_t base, uint64_t *returns,
	int maximum, xg_frame_reader read)
{
	uint64_t previous = 0, record[2];
	int count = 0;
	while (count < maximum && fp && !(fp & 7))
	{
		uint64_t host = xg_frame_host_address(fp, base);
		if (host <= previous || !read(host, record))
			break;
		returns[count++] = record[1];
		previous = host;
		fp = record[0];
	}
	return count;
}

#endif
