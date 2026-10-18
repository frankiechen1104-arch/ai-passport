#include "st_pet.h"
#include "st_work.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
static void persistence(void) {
  st_state_t s, loaded;
  uint8_t data[ST_SAVE_BYTES];
  st_init(&s, 0);
  s.coins = 1234;
  st_play(&s);
  assert(st_buy_outfit(&s, 4));
  s.sleeping = 1;
  st_advance(&s, 1800);
  st_encode(&s, data);
  assert(st_decode(&loaded, data, sizeof(data)));
  assert(loaded.coins == s.coins);
  assert(loaded.outfit == 4 && loaded.sleeping && loaded.sleep_seconds == 1800);
  assert(st_valid(&loaded));
  for (unsigned i = 0; i < ST_SAVE_BYTES; i++) {
    data[i] ^= 1;
    assert(!st_decode(&loaded, data, sizeof(data)));
    data[i] ^= 1;
  }
  assert(!st_decode(&loaded, data, 63));
  assert(!st_decode(&loaded, NULL, 64));
  loaded.hunger = -1;
  st_seal(&loaded);
  assert(!st_valid(&loaded));
  st_init(&s, 0);
  s.owned_outfits = 0;
  st_seal(&s);
  assert(!st_valid(&s));
}
static void economy(void) {
  st_state_t s;
  st_init(&s, 0);
  assert(!st_feed(&s, ST_FOOD_DRY));
  assert(st_work_finish(&s, 4, 3) == 15 && s.coins == 15 && s.high_score == 10);
  assert(st_feed(&s, ST_FOOD_DRY) && s.coins == 5 && s.hunger == 100);
  assert(!st_feed(&s, (st_food_t)-1));
  assert(!st_travel(&s, 8));
  assert(!st_equip(&s, 6));
  s.coins = 500;
  st_seal(&s);
  assert(st_buy_outfit(&s, 1));
  uint32_t coins = s.coins;
  assert(st_buy_outfit(&s, 1) && s.coins == coins);
  assert(!st_buy_outfit(&s, 5));
  s.joy_total = 250;
  assert(st_buy_outfit(&s, 5));
  for (unsigned i = 0; i < 8; i++) {
    s.energy = 100;
    s.coins = 500;
    s.joy_total = 600;
    assert(st_travel(&s, i));
  }
  assert(s.visited_places == 255);
  s.energy = 0;
  assert(!st_travel(&s, 0));
  s.coins = UINT32_MAX - 1;
  st_work_finish(&s, UINT32_MAX, UINT32_MAX);
  assert(s.coins == UINT32_MAX);
  assert(st_work_score(UINT32_MAX, 21) == UINT32_MAX);
}
static void timing(void) {
  st_state_t s;
  st_init(&s, 0);
  st_advance(&s, 3599);
  assert(s.hunger == 80);
  st_advance(&s, 1);
  assert(s.hunger == 78 && s.energy == 79);
  s.energy = 0;
  s.sleeping = 1;
  st_advance(&s, 3599);
  assert(s.energy == 0);
  st_advance(&s, 1);
  assert(s.energy == 12);
  st_advance(&s, 8 * 3600);
  assert(s.energy == 96 && s.sleep_seconds == 8 * 3600);
  st_advance(&s, 86400);
  assert(s.energy == 96 && s.hunger == 12);
  assert(st_is_sulking(&s));
  st_init(&s, 0);
  st_tick(&s, 86400 * 90LL);
  assert(s.last_epoch_s == 86400 * 90LL);
  assert(s.hunger == 0);
  st_tick(&s, 1);
  assert(s.hunger == 0);
  assert(st_valid(&s));
}
static void conveyor(void) {
  st_work_t w;
  st_round_start(&w, 10000);
  assert(st_round_remaining(&w, 10000) == 60);
  assert(!st_round_strike(&w, 9999));
  assert(!st_round_strike(&w, 11239));
  assert(w.hits == 0 && w.misses == 1);
  assert(st_round_strike(&w, 11240));
  assert(w.hits == 1 && w.combo == 1);
  assert(!st_round_strike(&w, 11240));
  assert(w.hits == 1 && w.combo == 0);
  st_round_update(&w, 12661);
  assert(w.next == 2 && w.combo == 0);
  assert(st_round_strike(&w, 13760));
  assert(w.hits == 2);
  st_round_update(&w, 69999);
  assert(w.running && st_round_remaining(&w, 69999) == 1);
  st_round_update(&w, 70000);
  assert(!w.running && st_round_remaining(&w, 70000) == 0);
  assert(!st_round_strike(&w, 70000));
  st_round_start(&w, 0);
  for (unsigned i = 0; i < ST_BRICKS; i++)
    assert(st_round_strike(&w, ST_BRICK_FIRST_MS + i * ST_BRICK_PERIOD_MS));
  assert(w.hits == 54 && w.best_combo == 54);
  st_round_update(&w, 60000);
  assert(!w.running);
  assert(st_work_score(w.hits, w.best_combo) == 94);
  st_round_start(&w, 0);
  st_round_update(&w, 60000);
  assert(!w.running && w.hits == 0 && w.misses == 54);
}
int main(void) {
  persistence();
  economy();
  timing();
  conveyor();
  puts("ST state, NVS wire format, economy and 60-second conveyor: PASS");
}
