#ifndef ST_PET_H
#define ST_PET_H
#include <stdbool.h>
#include <stdint.h>
#define ST_VERSION 2u
#define ST_TRIPS 8u
#define ST_OUTFITS 6u
#define ST_WORK_MS 60000u

typedef enum { ST_FOOD_DRY, ST_FOOD_GRILLED, ST_FOOD_FEAST } st_food_t;
typedef enum {
  ST_IDLE,
  ST_FEED,
  ST_PLAY,
  ST_BATH,
  ST_SLEEP,
  ST_WORK,
  ST_TRAVEL,
  ST_WARDROBE,
  ST_SULK
} st_scene_t;
typedef struct {
  uint32_t magic, version;
  int16_t hunger, mood, energy, clean;
  uint32_t coins, joy_total, high_score;
  uint8_t owned_outfits, visited_places, outfit;
  int64_t last_epoch_s;
  uint32_t active_seconds, sleep_seconds;
  uint8_t sleeping, destination;
  uint32_t checksum;
} st_state_t;
typedef struct {
  const char *name;
  uint16_t cost;
  uint8_t mood_gain, energy_cost;
  uint16_t unlock_joy;
} st_trip_t;
typedef struct {
  const char *name;
  uint16_t cost, unlock_joy;
} st_outfit_t;
extern const st_trip_t ST_TRIP_TABLE[ST_TRIPS];
extern const st_outfit_t ST_OUTFIT_TABLE[ST_OUTFITS];
void st_init(st_state_t *s, int64_t now);
bool st_valid(const st_state_t *s);
void st_seal(st_state_t *s);
void st_tick(st_state_t *s, int64_t now);
bool st_feed(st_state_t *s, st_food_t food);
void st_play(st_state_t *s);
void st_bath(st_state_t *s);
void st_sleep(st_state_t *s, int64_t seconds);
bool st_travel(st_state_t *s, uint8_t trip);
bool st_buy_outfit(st_state_t *s, uint8_t id);
bool st_equip(st_state_t *s, uint8_t id);
void st_work_start(st_state_t *s);
uint32_t st_work_score(uint32_t hits, uint32_t best_combo);
uint32_t st_work_finish(st_state_t *s, uint32_t hits, uint32_t best_combo);
bool st_is_sulking(const st_state_t *s);

#define ST_SAVE_BYTES 64u
void st_encode(const st_state_t *s, uint8_t data[ST_SAVE_BYTES]);
bool st_decode(st_state_t *s, const uint8_t *data, unsigned size);
void st_advance(st_state_t *s, uint32_t elapsed_seconds);

#endif
