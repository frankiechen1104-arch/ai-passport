#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef enum {
  ST_MOTION_NONE,
  ST_MOTION_FEED,
  ST_MOTION_PLAY,
  ST_MOTION_BATH,
  ST_MOTION_WAKE,
  ST_MOTION_HIT,
  ST_MOTION_MISS,
  ST_MOTION_TRAVEL,
  ST_MOTION_DRESS,
  ST_MOTION_CELEBRATE
} st_motion_t;
typedef enum {
  ST_EFFECT_NONE,
  ST_EFFECT_FISH,
  ST_EFFECT_HEARTS,
  ST_EFFECT_BUBBLES,
  ST_EFFECT_STARS,
  ST_EFFECT_DUST,
  ST_EFFECT_SLEEP,
  ST_EFFECT_CONFETTI
} st_effect_t;
typedef struct {
  unsigned frame;
  int x, y, angle;
  unsigned scale_x, scale_y;
  st_effect_t effect;
  unsigned progress;
} st_pose_t;
unsigned st_motion_duration(st_motion_t motion);
/* Milliseconds are monotonic. This function allocates nothing and has no UI
 * dependencies. */
st_pose_t st_animation_sample(st_motion_t motion, int64_t started_ms,
                              int64_t now_ms, bool sleeping, bool sulking,
                              bool working);
