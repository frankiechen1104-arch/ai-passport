#include "lvgl.h"
#include "st_ui.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const char *output_directory;
static unsigned char frame[240 * 320 * 2], draw[240 * 40 * 2];
static void flush(lv_display_t *d, const lv_area_t *a, uint8_t *pixels) {
  for (int y = a->y1; y <= a->y2; y++)
    memcpy(frame + (y * 240 + a->x1) * 2,
           pixels + (y - a->y1) * (a->x2 - a->x1 + 1) * 2,
           (a->x2 - a->x1 + 1) * 2);
  lv_display_flush_ready(d);
}
static void screenshot(const char *name) {
  lv_obj_invalidate(lv_screen_active());
  lv_refr_now(NULL);
  char path[256];
  snprintf(path, sizeof(path), "%s/%s.ppm", output_directory, name);
  FILE *f = fopen(path, "wb");
  assert(f);
  fprintf(f, "P6\n240 320\n255\n");
  for (unsigned i = 0; i < 240 * 320; i++) {
    unsigned p = frame[2 * i] | (frame[2 * i + 1] << 8);
    unsigned char rgb[] = {((p >> 11) & 31) * 255 / 31,
                           ((p >> 5) & 63) * 255 / 63, (p & 31) * 255 / 31};
    fwrite(rgb, 1, 3, f);
  }
  fclose(f);
}
static void layout(lv_obj_t *o) {
  if (lv_obj_has_flag(o, LV_OBJ_FLAG_HIDDEN))
    return;
  if (lv_obj_check_type(o, &lv_label_class)) {
    const char *text = lv_label_get_text(o);
    lv_point_t size;
    lv_text_get_size(&size, text, lv_obj_get_style_text_font(o, 0), 0, 0,
                     LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    if (size.x > lv_obj_get_width(o)) {
      fprintf(stderr, "Clipped text (%d > %d): %s\n", size.x,
              (int)lv_obj_get_width(o), text);
      exit(2);
    }
  }
  for (unsigned i = 0; i < lv_obj_get_child_count(o); i++)
    layout(lv_obj_get_child(o, i));
}
int main(int argc, char **argv) {
  assert(argc == 2);
  output_directory = argv[1];
  lv_init();
  lv_display_t *d = lv_display_create(240, 320);
  lv_display_set_color_format(d, LV_COLOR_FORMAT_RGB565);
  lv_display_set_buffers(d, draw, NULL, sizeof(draw),
                         LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(d, flush);
  st_ui_create();
  st_state_t s;
  st_init(&s, 0);
  s.coins = 10000;
  s.joy_total = 600;
  s.hunger = s.mood = s.energy = s.clean = 100;
  st_view_t v = {.battery = 100};
  st_work_t w = {0};
  for (unsigned page = 0; page <= PAGE_RESULT; page++) {
    v.page = page;
    unsigned count =
        (page == PAGE_HOME || page == PAGE_TRAVEL || page == PAGE_COLLECTION)
            ? 8
        : page == PAGE_WARDROBE ? 6
        : page == PAGE_FOOD     ? 3
                                : 1;
    for (unsigned i = 0; i < count; i++) {
      v.selected = i;
      st_ui_render(&s, &v, &w, 0);
      lv_obj_update_layout(lv_screen_active());
      layout(lv_screen_active());
      char name[40];
      snprintf(name, sizeof(name), "page_%u_%u", page, i);
      screenshot(name);
    }
  }
  s.coins = UINT32_MAX;
  s.joy_total = UINT32_MAX;
  v.page = PAGE_HOME;
  v.selected = 0;
  st_ui_render(&s, &v, &w, 0);
  lv_obj_update_layout(lv_screen_active());
  layout(lv_screen_active());
  screenshot("large_counts");
  v.page = PAGE_WORK;
  st_round_start(&w, 0);
  st_ui_render(&s, &v, &w, 1400);
  screenshot("game_hit_zone");
  /* Capture actual LVGL frames for a repeatable animation preview. */
  s.coins = 180;
  s.joy_total = 160;
  s.high_score = 94;
  v.reward = 25;
  int64_t origin = 100000;
  const st_motion_t motions[] = {
      ST_MOTION_NONE, ST_MOTION_FEED,     ST_MOTION_BATH,   ST_MOTION_PLAY,
      ST_MOTION_WAKE, ST_MOTION_DRESS,    ST_MOTION_TRAVEL, ST_MOTION_HIT,
      ST_MOTION_MISS, ST_MOTION_CELEBRATE};
  for (unsigned outfit = 0; outfit < 6; outfit++) {
    s.outfit = outfit;
    v.page = PAGE_HOME;
    v.selected = 0;
    st_ui_render(&s, &v, &w, origin - 500);
    st_ui_render(&s, &v, &w, origin);
    for (unsigned action = 0; action < 10; action++) {
      v.page = action == 7 || action == 8 ? PAGE_WORK
               : action == 9              ? PAGE_RESULT
                                          : PAGE_HOME;
      if (action == 4)
        v.page = PAGE_SLEEP;
      if (action == 1)
        v.page = PAGE_FOOD;
      if (action == 5)
        v.page = PAGE_WARDROBE;
      if (action == 7 || action == 8)
        st_round_start(&w, origin - 1400);
      if (action == 7)
        st_round_strike(&w, origin);
      if (action == 8)
        st_round_strike(&w, origin - 500);
      v.selected = action == 5 ? outfit : action == 3 ? 1 : action == 2 ? 2 : 0;
      v.destination = action == 6 ? 1 : 0;
      v.motion = action == 4 ? ST_MOTION_NONE : motions[action];
      v.motion_started_ms = origin;
      s.sleeping = action == 4;
      for (unsigned f = 0; f < (action == 0 ? 100u : 36u); f++) {
        int64_t time = origin + f * 80;
        st_ui_render(&s, &v, &w, time);
        lv_obj_update_layout(lv_screen_active());
        layout(lv_screen_active());
        if (outfit == 0 || action == 5) {
          char name[80];
          snprintf(name, sizeof(name), "anim_%u_%u_%03u", outfit, action, f);
          screenshot(name);
        }
        lv_tick_inc(80);
        lv_timer_handler();
      }
      origin += action == 0 ? 10000 : 4000;
    }
  }
  s.sleeping = 0;
  v.page = PAGE_WORK;
  v.motion = ST_MOTION_NONE;
  /* Exercise rendering under the same 24 KB LVGL heap for 3000 frames. */
  for (unsigned i = 0; i < 3000; i++) {
    st_ui_render(&s, &v, &w, origin + i * 20);
    lv_tick_inc(20);
    lv_timer_handler();
  }
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  printf("LVGL layout and 3000 frames: PASS; free %lu, largest %lu bytes\n",
         (unsigned long)m.free_size, (unsigned long)m.free_biggest_size);
  return 0;
}
