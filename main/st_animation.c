#include "st_animation.h"
static int triangle(int64_t time, unsigned period) {
  unsigned t = (unsigned)(time % period);
  return t < period / 2 ? (int)(t * 200 / (period / 2)) - 100
                        : 300 - (int)(t * 200 / (period / 2));
}
unsigned st_motion_duration(st_motion_t motion) {
  static const unsigned durations[] = {0,   2800, 2400, 3600, 1200,
                                       520, 560,  1800, 1400, 3000};
  return (unsigned)motion < sizeof(durations) / sizeof(durations[0])
             ? durations[motion]
             : 0;
}
st_pose_t st_animation_sample(st_motion_t motion, int64_t started, int64_t now,
                              bool sleeping, bool sulking, bool working) {
  if (now < 0)
    now = 0;
  st_pose_t p = {.scale_x = 256, .scale_y = 256};
  int breath = triangle(now, sleeping ? 3600 : 2400);
  p.scale_x = 256 + breath / 35;
  p.scale_y = 256 - breath / 50;
  unsigned idle = (unsigned)(now % 10000);
  p.frame = idle >= 3100 && idle < 3280 ? 1 : 0;
  if (idle >= 5800 && idle < 7600) {
    p.frame = ((idle - 5800) / 300) % 2 ? 3 : 2;
    p.angle = triangle(now, 1200) / 25;
  }
  if (sulking) {
    p.frame = 8;
    p.angle = triangle(now, 2800) / 25;
  }
  if (sleeping) {
    p.frame = 1;
    p.effect = ST_EFFECT_SLEEP;
  }
  if (working) {
    p.frame = (now % 1100) > 850 ? 9 : 0;
    p.effect = ST_EFFECT_NONE;
  }
  unsigned duration = st_motion_duration(motion);
  if (!duration || now < started || now - started >= duration)
    return p;
  unsigned t = (unsigned)(now - started);
  p.progress = t * 1000 / duration;
  switch (motion) {
  case ST_MOTION_FEED:
    p.frame = t < 550 ? 4 : ((t / 240) % 2 ? 5 : 4);
    p.effect = ST_EFFECT_FISH;
    p.angle = triangle(t, 700) / 20;
    break;
  case ST_MOTION_PLAY:
    p.frame = (t / 260) % 2 ? 3 : 2;
    p.y = -(triangle(t, 520) + 100) / 35;
    p.angle = triangle(t, 1040) / 15;
    p.effect = ST_EFFECT_HEARTS;
    break;
  case ST_MOTION_BATH:
    p.frame = (t / 220) % 2 ? 7 : 6;
    p.x = triangle(t, 440) / 30;
    p.angle = triangle(t, 880) / 18;
    p.effect = ST_EFFECT_BUBBLES;
    break;
  case ST_MOTION_WAKE:
    p.frame = t < 240 ? 1 : t < 600 ? 2 : 3;
    p.angle = triangle(t, 1000) / 25;
    p.effect = ST_EFFECT_STARS;
    break;
  case ST_MOTION_HIT:
    p.frame = t < 90 ? 9 : t < 300 ? 10 : 0;
    p.x = t < 300 ? (int)(t < 160 ? t / 30 : (300 - t) / 26) : 0;
    p.effect = ST_EFFECT_STARS;
    break;
  case ST_MOTION_MISS:
    p.frame = t < 320 ? 8 : 0;
    p.x = triangle(t, 140) / 28;
    p.effect = ST_EFFECT_DUST;
    break;
  case ST_MOTION_TRAVEL:
    p.frame = (t / 220) % 2 ? 2 : 3;
    p.x = triangle(t, 900) / 12;
    p.effect = ST_EFFECT_CONFETTI;
    break;
  case ST_MOTION_DRESS:
    p.frame = (t / 320) % 2 ? 2 : 3;
    p.angle = triangle(t, 700) / 12;
    p.effect = ST_EFFECT_STARS;
    break;
  case ST_MOTION_CELEBRATE:
    p.frame = (t / 320) % 2 ? 11 : 3;
    p.y = -(triangle(t, 640) + 100) / 32;
    p.angle = triangle(t, 1280) / 25;
    p.effect = ST_EFFECT_CONFETTI;
    break;
  default:
    break;
  }
  return p;
}
