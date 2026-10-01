/* A short touch gesture must survive a slow guest frame, but only one poll.
 * Physical controller state is merged separately; cancellation clears both
 * live and unread touch input. Kept independent of UIKit for regression tests. */
#ifndef XG_TOUCH_INPUT_H
#define XG_TOUCH_INPUT_H
#include <math.h>
#include <string.h>

struct xg_touch_pad { float axes[6]; unsigned int buttons; };
struct xg_touch_input
{
	struct xg_touch_pad current, pending;
};

static inline float xg_touch_stronger(float a, float b)
{
	return fabsf(a) >= fabsf(b) ? a : b;
}

static inline void xg_touch_publish(struct xg_touch_input *input, const struct xg_touch_pad *state)
{
	input->pending.buttons |= state->buttons & ~input->current.buttons;
	for (int axis = 0; axis < 6; axis++)
		if (state->axes[axis] != 0) input->pending.axes[axis] = state->axes[axis];
	input->current = *state;
}

static inline float xg_touch_axis(struct xg_touch_input *input, int axis)
{
	if (axis < 0 || axis >= 6) return 0;
	float value = input->current.axes[axis] != 0 ? input->current.axes[axis] : input->pending.axes[axis];
	input->pending.axes[axis] = 0;
	return value;
}

static inline int xg_touch_button(struct xg_touch_input *input, int button)
{
	if (button < 0 || button >= 32) return 0;
	int pressed = ((input->current.buttons | input->pending.buttons) >> button) & 1u;
	input->pending.buttons &= ~(1u << button);
	return pressed;
}

static inline void xg_touch_clear(struct xg_touch_input *input)
{
	memset(input, 0, sizeof(*input));
}
#endif
