/* One-finger roster paging. Keep a delivered key's release even on cancellation. */
#ifndef XG_SCOREBOARD_INPUT_H
#define XG_SCOREBOARD_INPUT_H
#include <math.h>
struct xg_scoreboard_input { float points; int pages, release; };
static inline void xg_scoreboard_clear(struct xg_scoreboard_input *input)
{
    input->points = 0;
    input->pages = 0;
}
static inline void xg_scoreboard_drag(struct xg_scoreboard_input *input, float dy)
{
    if (!isfinite(dy)) return;
    input->points = fmaxf(-640, fminf(640, input->points + dy));
    int pages = (int)(input->points / 80);
    input->points -= pages * 80;
    input->pages += pages;
    if (input->pages > 8) input->pages = 8;
    if (input->pages < -8) input->pages = -8;
}
/* direction: +1 Page Down, -1 Page Up, 0 no event. */
static inline int xg_scoreboard_next(struct xg_scoreboard_input *input, int *down)
{
    if (input->release) {
        int direction = input->release;
        input->release = 0;
        *down = 0;
        return direction;
    }
    if (!input->pages) return 0;
    int direction = input->pages > 0 ? 1 : -1;
    input->pages -= direction;
    input->release = direction;
    *down = 1;
    return direction;
}
#endif
