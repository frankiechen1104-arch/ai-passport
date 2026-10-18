#include "st_animation.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
  st_pose_t a = st_animation_sample(ST_MOTION_NONE, 0, 0, false, false, false);
  st_pose_t blink =
      st_animation_sample(ST_MOTION_NONE, 0, 3150, false, false, false);
  assert(a.frame == 0 && blink.frame == 1);
  assert(
      st_animation_sample(ST_MOTION_NONE, 0, 6100, false, false, false).frame ==
      3);
  assert(st_animation_sample(ST_MOTION_NONE, 0, 0, true, false, false).effect ==
         ST_EFFECT_SLEEP);
  assert(st_animation_sample(ST_MOTION_NONE, 0, 0, false, true, false).frame ==
         8);
  assert(st_animation_sample(ST_MOTION_HIT, 1000, 1100, false, false, true)
             .frame == 10);
  assert(st_animation_sample(ST_MOTION_FEED, 1000, 1600, false, false, false)
             .effect == ST_EFFECT_FISH);
  assert(st_animation_sample(ST_MOTION_BATH, 1000, 1300, false, false, false)
             .effect == ST_EFFECT_BUBBLES);
  assert(st_animation_sample(ST_MOTION_MISS, 1000, 1100, false, false, true)
             .frame == 8);
  /* An action is finite and future timestamps never underflow into a frame. */
  assert(st_animation_sample(ST_MOTION_HIT, 1000, 1520, false, false, false)
             .effect == ST_EFFECT_NONE);
  assert(st_animation_sample(ST_MOTION_HIT, 1000, 999, false, false, false)
             .effect == ST_EFFECT_NONE);
  assert(st_motion_duration((st_motion_t)99) == 0);
  for (unsigned kind = 0; kind <= ST_MOTION_CELEBRATE; kind++) {
    unsigned seen = 0;
    for (int64_t t = 0; t < 12000; t += 20) {
      st_pose_t p =
          st_animation_sample((st_motion_t)kind, 0, t, false, false, false);
      assert(p.frame < 12 && p.scale_x >= 250 && p.scale_x <= 262 &&
             p.scale_y >= 250 && p.scale_y <= 262);
      assert(p.x >= -10 && p.x <= 10 && p.y >= -7 && p.y <= 7 &&
             p.angle >= -9 && p.angle <= 9);
      assert(p.progress <= 1000);
      seen |= 1u << p.frame;
    }
    if (kind)
      assert(seen & (seen - 1));
  }
  puts("ST animation frame selection, timing, fallbacks and bounded "
       "transforms: PASS");
}
