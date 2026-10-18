#include "st_controller.h"
#include <assert.h>
#include <stdio.h>
static void key(st_controller_t *c, st_button_t btn, st_event_t ev,
                int64_t ms) {
  st_input_t e = {btn, ev, ms};
  st_controller_input(c, &e);
}
int main(void) {
  st_controller_t c;
  st_controller_init(&c);
  key(&c, ST_BTN_OK, ST_BTN_PRESS, 100);
  assert(c.view.page == PAGE_HOME);
  key(&c, ST_BTN_OK, ST_BTN_CLICK, 200);
  assert(c.view.page == PAGE_FOOD);
  c.pet.coins = 100;
  st_seal(&c.pet);
  key(&c, ST_BTN_OK, ST_BTN_PRESS, 300);
  key(&c, ST_BTN_OK, ST_BTN_LONG, 800);
  assert(c.view.page == PAGE_HOME && c.pet.coins == 100); // Back must not feed
  key(&c, ST_BTN_OK, ST_BTN_CLICK, 1000);
  assert(c.view.page == PAGE_FOOD);
  key(&c, ST_BTN_OK, ST_BTN_CLICK, 1100);
  assert(c.pet.coins == 90 && c.dirty);
  key(&c, ST_BTN_OK, ST_BTN_LONG, 2000);
  c.view.selected = 4;
  key(&c, ST_BTN_OK, ST_BTN_CLICK, 3000);
  assert(c.view.page == PAGE_WORK);
  key(&c, ST_BTN_OK, ST_BTN_PRESS, 4400);
  assert(c.work.hits == 1);
  key(&c, ST_BTN_OK, ST_BTN_CLICK, 4600);
  assert(c.view.page == PAGE_WORK); // no double hit
  st_controller_update(&c, 63000);
  assert(c.view.page == PAGE_RESULT && c.pet.coins == 98);
  st_controller_update(&c, 64000);
  assert(c.pet.coins == 98); // settle exactly once
  key(&c, ST_BTN_OK, ST_BTN_CLICK, 65000);
  assert(c.view.page == PAGE_WORK);
  key(&c, ST_BTN_OK, ST_BTN_PRESS, 66400);
  key(&c, ST_BTN_OK, ST_BTN_LONG, 66900);
  assert(c.view.page == PAGE_HOME);
  key(&c, ST_BTN_OK, ST_BTN_CLICK, 67000);
  assert(c.view.page == PAGE_FOOD); // no stale click suppression
  st_controller_update(&c, 126000);
  assert(c.pet.coins == 98); // abort has no payout
  key(&c, ST_BTN_OK, ST_BTN_LONG, 127000);
  c.view.selected = 3;
  key(&c, ST_BTN_OK, ST_BTN_CLICK, 128000);
  assert(c.pet.sleeping && c.view.page == PAGE_SLEEP);
  key(&c, ST_BTN_OK, ST_BTN_LONG, 129000);
  c.view.selected = 4;
  key(&c, ST_BTN_OK, ST_BTN_CLICK, 130000);
  assert(c.view.page == PAGE_HOME); // asleep cannot work
  c.view.selected = 3;
  key(&c, ST_BTN_OK, ST_BTN_CLICK, 131000);
  key(&c, ST_BTN_OK, ST_BTN_CLICK, 132000);
  assert(!c.pet.sleeping);
  key(&c, ST_BTN_UP, ST_BTN_CLICK, 133000);
  assert(c.view.selected == 7);
  c.view.selected = 0;
  key(&c, ST_BTN_UP, ST_BTN_CLICK, 134000);
  assert(c.view.selected == 7);
  puts("ST menu, long-back, sleep, strike/click and one-time reward: PASS");
}
