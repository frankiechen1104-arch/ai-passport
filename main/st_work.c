#include "st_work.h"
#include <string.h>
static int64_t target(unsigned i) {
  return ST_BRICK_FIRST_MS + (int64_t)i * ST_BRICK_PERIOD_MS;
}
void st_round_start(st_work_t *w, int64_t now_ms) {
  memset(w, 0, sizeof(*w));
  w->start_ms = now_ms;
  w->running = true;
}
void st_round_update(st_work_t *w, int64_t now_ms) {
  if (!w->running || now_ms < w->start_ms)
    return;
  int64_t elapsed = now_ms - w->start_ms;
  while (w->next < ST_BRICKS && elapsed > target(w->next) + ST_HIT_WINDOW_MS) {
    ++w->next;
    ++w->misses;
    w->combo = 0;
  }
  if (elapsed >= ST_WORK_MS)
    w->running = false;
}
bool st_round_strike(st_work_t *w, int64_t event_ms) {
  if (event_ms < w->start_ms || !w->running)
    return false;
  st_round_update(w, event_ms);
  if (!w->running)
    return false;
  int64_t elapsed = event_ms - w->start_ms;
  if (w->next < ST_BRICKS && elapsed >= target(w->next) - ST_HIT_WINDOW_MS &&
      elapsed <= target(w->next) + ST_HIT_WINDOW_MS) {
    ++w->next;
    ++w->hits;
    ++w->combo;
    if (w->combo > w->best_combo)
      w->best_combo = w->combo;
    return true;
  }
  ++w->misses;
  w->combo = 0;
  return false;
}
int st_round_remaining(const st_work_t *w, int64_t now_ms) {
  int64_t rest = ST_WORK_MS - (now_ms - w->start_ms);
  return rest <= 0 ? 0 : (int)((rest + 999) / 1000);
}
int st_round_brick_x(const st_work_t *w, int64_t now_ms, unsigned index) {
  int64_t delta = now_ms - w->start_ms - target(index);
  if (delta < -3000 || delta > 3000)
    return -1000;
  return 120 - (int)(delta * 100 / 1000);
}
