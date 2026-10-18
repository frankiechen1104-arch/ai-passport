#pragma once
#include "st_pet.h"
#include <stdbool.h>
#include <stdint.h>
#define ST_BRICK_PERIOD_MS 1100
#define ST_BRICK_FIRST_MS 1400
#define ST_HIT_WINDOW_MS 160
#define ST_BRICKS 54
/* All times are monotonic milliseconds, including queued key timestamps. */
typedef struct {
  int64_t start_ms;
  uint32_t hits, misses, combo, best_combo;
  unsigned next;
  bool running;
} st_work_t;
void st_round_start(st_work_t *w, int64_t now_ms);
void st_round_update(st_work_t *w, int64_t now_ms);
bool st_round_strike(st_work_t *w, int64_t event_ms);
int st_round_remaining(const st_work_t *w, int64_t now_ms);
int st_round_brick_x(const st_work_t *w, int64_t now_ms, unsigned index);
