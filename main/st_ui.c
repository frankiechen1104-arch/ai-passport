#include "st_ui.h"
#include "lvgl.h"
#include "st_animation_art.h"
#include "st_art.h"
#include <stdio.h>
#include <string.h>
LV_FONT_DECLARE(st_font_16);
extern const uint32_t st_font_codepoints[];
extern const unsigned st_font_codepoint_count;
static lv_obj_t *bg, *pet, *title, *battery, *stats, *card, *choice, *detail,
    *note, *hint;
static lv_obj_t *belt, *zone, *bricks[3], *score, *score_panel;
static lv_obj_t *particles[10], *snooze[3], *fish, *fish_tail, *fish_eye;
static int displayed_bg = -1, target_bg = -1, displayed_outfit = -1,
           target_outfit = -1;
static int64_t bg_changed, outfit_changed;
static void set_text(lv_obj_t *o, const char *text) {
  if (strcmp(lv_label_get_text(o), text))
    lv_label_set_text(o, text);
}
static const char *const menus[] = {"喂食", "玩耍", "洗澡", "睡觉",
                                    "搬砖", "旅行", "换装", "收藏"};
static const char *const trips[] = {
    "格里菲斯天文台", "圣莫尼卡",     "威尼斯海滩", "一号公路",
    "约书亚树",       "六十六号公路", "棕榈泉",     "大熊湖"};
static const char *const outfits[] = {"完整蓝鱼头帽", "宽松卫衣",   "工装背心",
                                      "渔夫帽配饰",   "牛仔背带裤", "小墨镜"};
static const char *const foods[] = {"小鱼干", "烤鱼", "海鲜大餐"};
static void compact_number(char text[16], uint32_t value) {
  if (value >= 1000000)
    snprintf(text, 16, "%luM", (unsigned long)value / 1000000);
  else if (value >= 10000)
    snprintf(text, 16, "%luK", (unsigned long)value / 1000);
  else
    snprintf(text, 16, "%lu", (unsigned long)value);
}
static lv_obj_t *label(lv_obj_t *parent, int x, int y, int width) {
  lv_obj_t *o = lv_label_create(parent);
  lv_obj_set_pos(o, x, y);
  lv_obj_set_width(o, width);
  lv_obj_set_style_text_font(o, &st_font_16, 0);
  lv_obj_set_style_text_color(o, lv_color_hex(0xffffff), 0);
  lv_label_set_long_mode(o, LV_LABEL_LONG_CLIP);
  return o;
}
static lv_obj_t *box(lv_obj_t *parent, int x, int y, int w, int h,
                     uint32_t color) {
  lv_obj_t *o = lv_obj_create(parent);
  lv_obj_remove_style_all(o);
  lv_obj_set_pos(o, x, y);
  lv_obj_set_size(o, w, h);
  lv_obj_set_style_bg_color(o, lv_color_hex(color), 0);
  lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
  lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
  return o;
}
static void visible(lv_obj_t *o, bool show) {
  if (show)
    lv_obj_remove_flag(o, LV_OBJ_FLAG_HIDDEN);
  else
    lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
}
static void animate_effects(const st_pose_t *p, int64_t now, int x, int y,
                            unsigned scale, bool game) {
  bool bubbles = p->effect == ST_EFFECT_BUBBLES;
  bool emit = p->effect != ST_EFFECT_NONE && p->effect != ST_EFFECT_FISH &&
              p->effect != ST_EFFECT_SLEEP;
  for (unsigned i = 0; i < 10; i++) {
    lv_obj_t *o = particles[i];
    visible(o, emit);
    if (!emit)
      continue;
    unsigned phase = ((unsigned)(now % 1600) + i * 157) % 1600;
    int dx = (int)((i * 37) % 100) - 50;
    int px = game ? 120 + dx * (int)(phase + 200) / 1700
                  : x + 64 * (int)scale / 256 + dx;
    int py = game ? 185 - (int)phase / 28
                  : y + 155 * (int)scale / 256 - (int)phase / 14;
    lv_obj_set_pos(o, px, py);
    unsigned size = bubbles ? 5 + (i % 4) * 3 : 3 + i % 4;
    lv_obj_set_size(o, size, size);
    lv_obj_set_style_radius(o, bubbles ? LV_RADIUS_CIRCLE : 2, 0);
    lv_obj_set_style_bg_color(
        o,
        lv_color_hex(bubbles                         ? 0x99eaff
                     : p->effect == ST_EFFECT_HEARTS ? 0xff8ba6
                     : p->effect == ST_EFFECT_DUST   ? 0xb7b8aa
                                                     : 0xffd575),
        0);
    lv_obj_set_style_bg_opa(o, bubbles ? 90 : 230, 0);
    lv_obj_set_style_border_width(o, bubbles ? 1 : 0, 0);
    lv_obj_set_style_border_color(o, lv_color_hex(0xe8ffff), 0);
    lv_obj_set_style_opa(o, 255 - phase * 200 / 1600, 0);
  }
  for (unsigned i = 0; i < 3; i++) {
    visible(snooze[i], p->effect == ST_EFFECT_SLEEP);
    unsigned phase = ((unsigned)(now % 2100) + i * 700) % 2100;
    lv_obj_set_pos(snooze[i], x + 95 + (int)phase / 180,
                   y + 46 - (int)phase / 75);
    lv_obj_set_style_opa(snooze[i], 255 - phase * 220 / 2100, 0);
  }
  bool feed = p->effect == ST_EFFECT_FISH && p->progress < 190;
  visible(fish, feed);
  visible(fish_tail, feed);
  visible(fish_eye, feed);
  if (feed) {
    int fx = x + 120 - (int)p->progress / 3,
        fy = y + 115 - (int)p->progress / 3;
    lv_obj_set_pos(fish, fx, fy);
    lv_obj_set_pos(fish_tail, fx + 16, fy);
    lv_obj_set_pos(fish_eye, fx + 3, fy + 3);
  }
}
void st_ui_create(void) {
  lv_obj_t *s = lv_screen_active();
  lv_obj_remove_style_all(s);
  lv_obj_set_style_bg_color(s, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(s, 255, 0);
  lv_obj_remove_flag(s, LV_OBJ_FLAG_SCROLLABLE);
  bg = lv_image_create(s);
  lv_image_set_src(bg, st_backgrounds[0]);
  pet = lv_image_create(s);
  lv_image_set_src(pet, st_animation_frames[0][0]);
  for (unsigned i = 0; i < 10; i++)
    particles[i] = box(s, 0, 0, 7, 7, 0xffd575);
  for (unsigned i = 0; i < 3; i++) {
    snooze[i] = label(s, 0, 0, 18);
    set_text(snooze[i], "z");
  }
  fish = box(s, 0, 0, 18, 9, 0xe6f7ff);
  lv_obj_set_style_radius(fish, LV_RADIUS_CIRCLE, 0);
  fish_tail = box(s, 0, 0, 6, 9, 0x7ed3ec);
  fish_eye = box(s, 0, 0, 2, 2, 0x163344);
  lv_obj_set_pos(pet, 56, 61);
  box(s, 14, 6, 212, 52, 0x102536);
  title = label(s, 22, 10, 154);
  battery = label(s, 177, 10, 42);
  stats = label(s, 22, 32, 200);
  belt = box(s, 0, 172, 240, 38, 0x243345);
  zone = box(s, 104, 165, 32, 52, 0x26806e);
  lv_obj_set_style_border_width(zone, 2, 0);
  lv_obj_set_style_border_color(zone, lv_color_hex(0x87ffd7), 0);
  for (unsigned i = 0; i < 3; i++) {
    bricks[i] = box(s, 0, 181, 24, 19, 0xe69855);
    lv_obj_set_style_border_width(bricks[i], 2, 0);
    lv_obj_set_style_border_color(bricks[i], lv_color_hex(0xffd6a1), 0);
  }
  score_panel = box(s, 14, 73, 212, 60, 0x102536);
  score = label(score_panel, 10, 6, 192);
  card = box(s, 14, 232, 212, 61, 0x102536);
  lv_obj_set_style_radius(card, 12, 0);
  choice = label(card, 10, 5, 192);
  detail = label(card, 10, 25, 192);
  note = label(s, 18, 212, 204);
  lv_obj_set_style_text_align(note, LV_TEXT_ALIGN_CENTER, 0);
  hint = label(s, 18, 296, 204);
  lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
  /* Verify actual compiled bitmap coverage, never suppress placeholders. */
  for (unsigned i = 0; i < st_font_codepoint_count; i++) {
    lv_font_glyph_dsc_t glyph = {0};
    LV_ASSERT_MSG(
        lv_font_get_glyph_dsc(&st_font_16, &glyph, st_font_codepoints[i], 0) &&
            !glyph.is_placeholder,
        "ST font missing glyph");
  }
}
void st_ui_render(const st_state_t *s, const st_view_t *v, const st_work_t *w,
                  int64_t now) {
  unsigned destination = v->destination;
  if (v->page == PAGE_TRAVEL || v->page == PAGE_COLLECTION)
    destination = v->selected;
  if (displayed_bg < 0) {
    displayed_bg = target_bg = destination;
    lv_image_set_src(bg, st_backgrounds[destination]);
  }
  if (target_bg != (int)destination) {
    target_bg = destination;
    bg_changed = now;
  }
  int64_t bg_age = now - bg_changed;
  unsigned bg_opa = 255;
  if (displayed_bg != target_bg && bg_age >= 180) {
    displayed_bg = target_bg;
    lv_image_set_src(bg, st_backgrounds[displayed_bg]);
  }
  if (bg_changed && bg_age < 420)
    bg_opa = bg_age < 180 ? 255 - (unsigned)bg_age * 240 / 180
                          : 15 + (unsigned)(bg_age - 180) * 240 / 240;
  lv_obj_set_style_opa(bg, bg_opa, 0);
  unsigned outfit = v->page == PAGE_WARDROBE ? v->selected : s->outfit;
  if (displayed_outfit < 0)
    displayed_outfit = target_outfit = outfit;
  if (target_outfit != (int)outfit) {
    target_outfit = outfit;
    outfit_changed = now;
  }
  int64_t outfit_age = now - outfit_changed;
  unsigned pet_opa = 255;
  if (displayed_outfit != target_outfit && outfit_age >= 140)
    displayed_outfit = target_outfit;
  if (outfit_changed && outfit_age < 320)
    pet_opa = outfit_age < 140 ? 255 - (unsigned)outfit_age * 200 / 140
                               : 55 + (unsigned)(outfit_age - 140) * 200 / 180;
  bool game = v->page == PAGE_WORK;
  bool result = v->page == PAGE_RESULT;
  st_pose_t pose = st_animation_sample(v->motion, v->motion_started_ms, now,
                                       s->sleeping, st_is_sulking(s), game);
  visible(pet, true);
  lv_image_set_src(pet, st_animation_frames[displayed_outfit][pose.frame]);
  int base_x = game ? 14 : result ? 74 : 56, base_y = game ? 66 : 61;
  unsigned scale = game ? 154 : result ? 184 : 256;
  lv_image_set_pivot(pet, 64, 164);
  /* Keep the feet anchored across breathing, gestures and game-sized sprites.
   */
  lv_obj_set_pos(pet, base_x - (256 - (int)scale) * 64 / 256 + pose.x,
                 base_y - (256 - (int)scale) * 164 / 256 + pose.y);
  lv_image_set_scale_x(pet, scale * pose.scale_x / 256);
  lv_image_set_scale_y(pet, scale * pose.scale_y / 256);
  lv_image_set_rotation(pet, pose.angle * 10);
  lv_obj_set_style_opa(pet, pet_opa, 0);
  animate_effects(&pose, now, base_x, base_y, scale, game);
  lv_obj_set_pos(score_panel, game ? 99 : 14, game ? 67 : 177);
  lv_obj_set_size(score_panel, game ? 127 : 212, game ? 72 : 34);
  lv_obj_set_width(score, game ? 111 : 192);
  visible(belt, game);
  visible(zone, game);
  visible(score, game || v->page == PAGE_RESULT);
  visible(score_panel, game || v->page == PAGE_RESULT);
  for (unsigned i = 0; i < 3; i++) {
    int x = st_round_brick_x(w, now, w->next + i);
    visible(bricks[i], game && w->next + i < ST_BRICKS && x >= -24 && x < 240);
    if (game)
      lv_obj_set_x(bricks[i], x);
  }
  char text[160];
  if (v->battery < 0)
    set_text(battery, "--%");
  else {
    snprintf(text, sizeof(text), "%d%%", v->battery);
    set_text(battery, text);
  }
  snprintf(text, sizeof(text), "饱%d 乐%d 力%d 洁%d", s->hunger, s->mood,
           s->energy, s->clean);
  set_text(stats, text);
  set_text(hint, game ? "绿框按确定 · 长按退出" : "上下选 · 确定 · 长按返回");
  const char *notice = v->save_error ? "存档失败 · 将重试" : v->notice;
  if (!notice || !*notice)
    notice = st_is_sulking(s) ? "ST 闹别扭啦 · 需要照顾" : "";
  set_text(note, notice);
  switch (v->page) {
  case PAGE_HOME:
    set_text(title, "ST 水獭护照");
    snprintf(text, sizeof(text), "%u/8  %s", v->selected + 1,
             menus[v->selected]);
    set_text(choice, text);
    char coins_text[16], joy_text[16];
    compact_number(coins_text, s->coins);
    compact_number(joy_text, s->joy_total);
    snprintf(text, sizeof(text), "金币 %s   快乐 %s", coins_text, joy_text);
    set_text(detail, text);
    break;
  case PAGE_FOOD:
    set_text(title, "喂食小水獭");
    snprintf(text, sizeof(text), "%u/3  %s", v->selected + 1,
             foods[v->selected]);
    set_text(choice, text);
    snprintf(text, sizeof(text), "花费 %u 金币 · 饱食 +%u",
             (unsigned[]){10, 25, 50}[v->selected],
             (unsigned[]){25, 45, 70}[v->selected]);
    set_text(detail, text);
    break;
  case PAGE_TRAVEL: {
    const st_trip_t *t = &ST_TRIP_TABLE[v->selected];
    set_text(title, "加州旅行");
    snprintf(text, sizeof(text), "%u/8 %s", v->selected + 1,
             trips[v->selected]);
    set_text(choice, text);
    if (s->joy_total < t->unlock_joy)
      snprintf(text, sizeof(text), "快乐 %u 解锁 · 当前 %lu", t->unlock_joy,
               (unsigned long)s->joy_total);
    else
      snprintf(text, sizeof(text), "币%u  力-%u  乐+%u", t->cost,
               t->energy_cost, t->mood_gain);
    set_text(detail, text);
    break;
  }
  case PAGE_WARDROBE: {
    const st_outfit_t *o = &ST_OUTFIT_TABLE[v->selected];
    set_text(title, "ST 的衣橱");
    snprintf(text, sizeof(text), "%u/6 %s", v->selected + 1,
             outfits[v->selected]);
    set_text(choice, text);
    if (s->owned_outfits & (1u << v->selected))
      set_text(detail, "已拥有 · 确定穿上");
    else if (s->joy_total < o->unlock_joy) {
      snprintf(text, sizeof(text), "快乐 %u 解锁", o->unlock_joy);
      set_text(detail, text);
    } else {
      snprintf(text, sizeof(text), "金币 %u · 确定购买", o->cost);
      set_text(detail, text);
    }
    break;
  }
  case PAGE_COLLECTION:
    set_text(title, "旅行收藏册");
    set_text(choice, trips[v->selected]);
    set_text(detail, s->visited_places & (1u << v->selected)
                         ? "已打卡 · 确定查看"
                         : "未打卡 · 去旅行点亮");
    break;
  case PAGE_SLEEP:
    set_text(title, "ST 正在睡觉");
    set_text(choice, "确定叫醒小水獭");
    snprintf(text, sizeof(text), "睡 %lu 分 · 力 +12/小时",
             (unsigned long)s->sleep_seconds / 60);
    set_text(detail, text);
    break;
  case PAGE_WORK:
    set_text(title, "六十秒搬砖");
    snprintf(text, sizeof(text), "%d 秒\n命中 %lu\n连击 %lu",
             st_round_remaining(w, now), (unsigned long)w->hits,
             (unsigned long)w->combo);
    set_text(score, text);
    set_text(choice, "入绿框时按确定");
    snprintf(text, sizeof(text), "最佳连击 %lu · 失误 %lu",
             (unsigned long)w->best_combo, (unsigned long)w->misses);
    set_text(detail, text);
    break;
  case PAGE_RESULT:
    set_text(title, "搬砖完成啦");
    snprintf(text, sizeof(text), "得分 %lu · 最高 %lu",
             (unsigned long)st_work_score(w->hits, w->best_combo),
             (unsigned long)s->high_score);
    set_text(score, text);
    snprintf(text, sizeof(text), "赚到 %lu 金币", (unsigned long)v->reward);
    set_text(choice, text);
    set_text(detail, "确定再来一局 · 长按返回");
    break;
  }
}
