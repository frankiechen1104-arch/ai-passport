#include "st_controller.h"
#include <string.h>
static void notice(st_controller_t *c, const char *text, int64_t now) {
  c->view.notice = text;
  c->notice_until = now + 2500;
}
void st_controller_dirty(st_controller_t *c, int64_t now) {
  c->dirty = true;
  if (!c->save_after)
    c->save_after = now + 1500;
}
static void home(st_controller_t *c) {
  c->view.page = PAGE_HOME;
  c->view.selected = c->home_selection;
}
static void start_work(st_controller_t *c, int64_t now) {
  if (c->pet.energy < 10) {
    notice(c, "体力不足 · 先睡一觉", now);
    return;
  }
  st_work_start(&c->pet);
  st_controller_dirty(c, now);
  st_round_start(&c->work, now);
  c->view.page = PAGE_WORK;
  c->view.notice = "";
}
void st_controller_input(st_controller_t *c, const st_input_t *e) {
  int64_t now = e->ms;
  if (e->btn == ST_BTN_OK && e->event == ST_BTN_CLICK && c->ignore_ok_click) {
    c->ignore_ok_click = false;
    return;
  }
  if (e->btn == ST_BTN_OK && e->event == ST_BTN_LONG) {
    c->ignore_ok_click = false;
    if (c->view.page == PAGE_WORK) {
      c->work.running = false;
      notice(c, "已退出 · 本局无奖励", now);
    }
    home(c);
    return;
  }
  if (c->view.page == PAGE_WORK) {
    if (e->btn == ST_BTN_OK && e->event == ST_BTN_PRESS) {
      c->ignore_ok_click = true;
      notice(c, st_round_strike(&c->work, now) ? "命中！" : "再准一点", now);
    }
    return;
  }
  /* Menus confirm on CLICK; a long back press never purchases an item. */
  if (e->event != ST_BTN_CLICK)
    return;
  if (e->btn == ST_BTN_UP || e->btn == ST_BTN_DOWN) {
    unsigned count =
        c->view.page == PAGE_HOME                                          ? 8
        : c->view.page == PAGE_FOOD                                        ? 3
        : c->view.page == PAGE_WARDROBE                                    ? 6
        : (c->view.page == PAGE_TRAVEL || c->view.page == PAGE_COLLECTION) ? 8
                                                                           : 0;
    if (count)
      c->view.selected =
          (c->view.selected + (e->btn == ST_BTN_UP ? count - 1 : 1)) % count;
    if (c->view.page == PAGE_HOME)
      c->home_selection = c->view.selected;
    return;
  }
  if (e->btn != ST_BTN_OK)
    return;
  switch (c->view.page) {
  case PAGE_HOME:
    if (c->pet.sleeping && c->view.selected != 3) {
      notice(c, "ST 在睡觉 · 请先叫醒", now);
      break;
    }
    switch (c->view.selected) {
    case 0:
      c->view.page = PAGE_FOOD;
      c->view.selected = 0;
      break;
    case 1:
      st_play(&c->pet);
      st_controller_dirty(c, now);
      notice(c, "一起玩耍 · 快乐 +20", now);
      break;
    case 2:
      st_bath(&c->pet);
      st_controller_dirty(c, now);
      notice(c, "洗香香 · 清洁满满", now);
      break;
    case 3:
      c->pet.sleeping = 1;
      st_seal(&c->pet);
      st_controller_dirty(c, now);
      c->view.page = PAGE_SLEEP;
      break;
    case 4:
      start_work(c, now);
      break;
    case 5:
      c->view.page = PAGE_TRAVEL;
      c->view.selected = c->view.destination;
      break;
    case 6:
      c->view.page = PAGE_WARDROBE;
      c->view.selected = c->pet.outfit;
      break;
    case 7:
      c->view.page = PAGE_COLLECTION;
      c->view.selected = 0;
      break;
    }
    break;
  case PAGE_FOOD:
    if (st_feed(&c->pet, (st_food_t)c->view.selected)) {
      st_controller_dirty(c, now);
      notice(c, "好吃 · 饱饱的", now);
    } else
      notice(c, "金币不足 · 去搬砖吧", now);
    break;
  case PAGE_TRAVEL:
    if (st_travel(&c->pet, c->view.selected)) {
      c->view.destination = c->view.selected;
      st_controller_dirty(c, now);
      home(c);
      notice(c, "旅行打卡成功", now);
    } else
      notice(c, "金币或体力不足或未解锁", now);
    break;
  case PAGE_WARDROBE:
    if (st_buy_outfit(&c->pet, c->view.selected)) {
      st_controller_dirty(c, now);
      notice(c, "穿上啦 · 鱼头帽保留", now);
    } else
      notice(c, "金币不足或快乐未解锁", now);
    break;
  case PAGE_COLLECTION:
    if (c->pet.visited_places & (1u << c->view.selected)) {
      c->view.destination = c->view.selected;
      home(c);
      notice(c, "旅行回忆", now);
    }
    break;
  case PAGE_SLEEP:
    c->pet.sleeping = 0;
    c->pet.sleep_seconds = 0;
    st_seal(&c->pet);
    st_controller_dirty(c, now);
    home(c);
    notice(c, "睡醒啦", now);
    break;
  case PAGE_RESULT:
    start_work(c, now);
    break;
  default:
    break;
  }
}

void st_controller_init(st_controller_t *c) {
  memset(c, 0, sizeof(*c));
  st_init(&c->pet, 0);
  c->view.battery = -1;
}
void st_controller_update(st_controller_t *c, int64_t now) {
  if (c->view.page == PAGE_WORK) {
    st_round_update(&c->work, now);
    if (!c->work.running) {
      c->view.reward =
          st_work_finish(&c->pet, c->work.hits, c->work.best_combo);
      c->view.page = PAGE_RESULT;
      st_controller_dirty(c, now);
      notice(c, "辛苦啦 · 金币到账", now);
    }
  }
  if (now >= c->notice_until)
    c->view.notice = "";
}
