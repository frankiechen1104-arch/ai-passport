#include "poetry_input.h"

bool poetry_input_event(poetry_input_t *s, poetry_key_t key,
                        poetry_raw_event_t ev, uint64_t ms, bool allowed,
                        poetry_press_t *out)
{
    if (key > POETRY_OK) return false;
    if (ev == POETRY_RAW_PRESS) {
        if (s->down && s->key == key) return false;
        *s = (poetry_input_t){ .down = true, .key = key, .started_ms = ms, .blocked = !allowed };
        return false;
    }
    if (!s->down || s->key != key) return false;
    if (ev == POETRY_RAW_LONG) {
        if (s->held || s->blocked || !allowed || ms < s->started_ms || ms - s->started_ms < 500) return false;
        s->held = true;
        *out = POETRY_LONG;
        return true;
    }
    if (ev != POETRY_RAW_RELEASE) return false;
    bool fire = !s->held && !s->blocked && allowed && ms >= s->started_ms && ms - s->started_ms >= 30;
    s->down = false;
    /* The BSP's repeat-tap state does not emit LONG for a second rapid press.
     * Elapsed grading also handles a release queued while the worker was busy. */
    if (fire) *out = ms - s->started_ms >= 500 ? POETRY_LONG : POETRY_SHORT;
    return fire;
}
