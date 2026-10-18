#include "st_pet.h"
#include <limits.h>
#include <stddef.h>
#include <string.h>
#define MAGIC 0x53544F54u
#define MAX_OFFLINE_SECONDS (30LL * 86400)
const st_trip_t ST_TRIP_TABLE[ST_TRIPS] = {
    {"Griffith Observatory", 0, 30, 15, 0}, {"Santa Monica", 50, 35, 20, 0},
    {"Venice Beach", 30, 30, 15, 0},        {"Highway 1", 20, 25, 10, 0},
    {"Joshua Tree", 40, 35, 25, 0},         {"Route 66", 0, 40, 20, 150},
    {"Palm Springs", 0, 45, 10, 350},       {"Big Bear Lake", 0, 50, 30, 600}};
const st_outfit_t ST_OUTFIT_TABLE[ST_OUTFITS] = {
    {"Blue fish hat", 0, 0},    {"Oversized hoodie", 60, 0},
    {"Utility vest", 80, 0},    {"Bucket hat", 50, 0},
    {"Denim overalls", 120, 0}, {"Sunglasses", 0, 250}};
static int16_t clamp(int32_t n) {
  return (int16_t)(n < 0 ? 0 : n > 100 ? 100 : n);
}
static uint32_t plus(uint32_t a, uint32_t b) {
  return UINT32_MAX - a < b ? UINT32_MAX : a + b;
}
static uint32_t check(const st_state_t *s) {
  const unsigned char *p = (const unsigned char *)s;
  uint32_t h = 2166136261u;
  for (unsigned i = 0; i < offsetof(st_state_t, checksum); ++i) {
    h ^= p[i];
    h *= 16777619u;
  }
  return h;
}
void st_seal(st_state_t *s) { s->checksum = check(s); }
bool st_valid(const st_state_t *s) {
  return s && s->magic == MAGIC && s->version == ST_VERSION &&
         s->checksum == check(s) && s->outfit < ST_OUTFITS &&
         (s->owned_outfits & 1u) && (s->owned_outfits & (1u << s->outfit)) &&
         !(s->owned_outfits & 0xc0u) && s->sleeping <= 1 &&
         s->destination < ST_TRIPS && s->active_seconds < 3600 &&
         s->sleep_seconds <= 8 * 3600 && s->hunger <= 100 && s->mood <= 100 &&
         s->energy <= 100 && s->clean <= 100 && s->hunger >= 0 &&
         s->mood >= 0 && s->energy >= 0 && s->clean >= 0;
}
void st_init(st_state_t *s, int64_t now) {
  memset(s, 0, sizeof(*s));
  s->magic = MAGIC;
  s->version = ST_VERSION;
  s->hunger = s->mood = s->energy = s->clean = 80;
  s->owned_outfits = 1u;
  s->last_epoch_s = now;
  st_seal(s);
}
void st_tick(st_state_t *s, int64_t now) {
  if (now <= s->last_epoch_s)
    return;
  int64_t delta = now - s->last_epoch_s;
  if (delta > MAX_OFFLINE_SECONDS)
    delta = MAX_OFFLINE_SECONDS;
  // Tick in complete hours to avoid frequent small truncations. Advance clock
  // in hour increments; cap catch-up to 30 days to prevent extreme decay after
  // clock corrections.
  int64_t hours = delta / 3600;
  if (hours) {
    s->hunger = clamp(s->hunger - (int32_t)(hours * 2));
    s->mood = clamp(s->mood - (int32_t)(hours));
    s->energy = clamp(s->energy - (int32_t)(hours));
    s->clean = clamp(s->clean - (int32_t)(hours));
    s->last_epoch_s = now - (delta % 3600);
  }
  st_seal(s);
}
static void joy(st_state_t *s, uint32_t n) {
  s->joy_total = plus(s->joy_total, n);
}
bool st_feed(st_state_t *s, st_food_t f) {
  static const uint16_t cost[] = {10, 25, 50};
  static const uint8_t gain[] = {25, 45, 70};
  if ((unsigned)f > 2u || s->coins < cost[f])
    return false;
  s->coins -= cost[f];
  s->hunger = clamp(s->hunger + gain[f]);
  s->energy = clamp(s->energy + 5);
  st_seal(s);
  return true;
}
void st_play(st_state_t *s) {
  s->mood = clamp(s->mood + 20);
  s->energy = clamp(s->energy - 8);
  joy(s, 20);
  st_seal(s);
}
void st_bath(st_state_t *s) {
  s->clean = 100;
  st_seal(s);
}
void st_sleep(st_state_t *s, int64_t sec) {
  if (sec < 0)
    sec = 0;
  if (sec > 8 * 3600)
    sec = 8 * 3600;
  s->energy = clamp(s->energy + (int32_t)(sec / 3600) * 12);
  st_seal(s);
}
bool st_travel(st_state_t *s, uint8_t i) {
  if (i >= ST_TRIPS)
    return false;
  const st_trip_t *t = &ST_TRIP_TABLE[i];
  if (s->joy_total < t->unlock_joy || s->coins < t->cost ||
      s->energy < t->energy_cost)
    return false;
  s->coins -= t->cost;
  s->mood = clamp(s->mood + t->mood_gain);
  s->energy = clamp(s->energy - t->energy_cost);
  s->destination = i;
  s->visited_places |= (uint8_t)(1u << i);
  joy(s, t->mood_gain);
  st_seal(s);
  return true;
}
bool st_equip(st_state_t *s, uint8_t i) {
  if (i >= ST_OUTFITS || !(s->owned_outfits & (1u << i)))
    return false;
  s->outfit = i;
  st_seal(s);
  return true;
}
bool st_buy_outfit(st_state_t *s, uint8_t i) {
  if (i >= ST_OUTFITS)
    return false;
  if (s->owned_outfits & (1u << i))
    return st_equip(s, i);
  const st_outfit_t *o = &ST_OUTFIT_TABLE[i];
  if (s->joy_total < o->unlock_joy || s->coins < o->cost)
    return false;
  s->coins -= o->cost;
  s->owned_outfits |= (uint8_t)(1u << i);
  s->outfit = i;
  s->mood = clamp(s->mood + 8);
  joy(s, 8);
  st_seal(s);
  return true;
}
void st_work_start(st_state_t *s) {
  s->energy = clamp(s->energy - 10);
  st_seal(s);
}
uint32_t st_work_score(uint32_t hits, uint32_t best_combo) {
  // simple bounded combo multiplier; best_combo provides a bonus capped to
  // prevent overflow
  uint32_t extra = best_combo > 20 ? 20 : best_combo;
  return hits > UINT32_MAX - extra * 2 ? UINT32_MAX : hits + extra * 2;
}
uint32_t st_work_finish(st_state_t *s, uint32_t hits, uint32_t best_combo) {
  uint32_t points = st_work_score(hits, best_combo);
  uint32_t earned = plus(5, points);
  s->coins = plus(s->coins, earned);
  if (points > s->high_score)
    s->high_score = points;
  st_seal(s);
  return earned;
}
bool st_is_sulking(const st_state_t *s) {
  return s->hunger < 20 || s->mood < 20 || s->energy < 20 || s->clean < 20;
}

/* Session time is trusted only while powered. A cold boot earns no offline
 * time. */
void st_advance(st_state_t *s, uint32_t seconds) {
  uint64_t total = (uint64_t)s->active_seconds + seconds;
  uint32_t hours = total / 3600;
  if (hours > 720)
    hours = 720;
  s->active_seconds = total % 3600;
  s->hunger = clamp(s->hunger - (int32_t)hours * 2);
  s->mood = clamp(s->mood - (int32_t)hours);
  s->clean = clamp(s->clean - (int32_t)hours);
  if (s->sleeping) {
    uint32_t before = s->sleep_seconds / 3600;
    uint64_t slept = (uint64_t)s->sleep_seconds + seconds;
    s->sleep_seconds = slept > 8 * 3600 ? 8 * 3600 : slept;
    s->energy =
        clamp(s->energy + 12 * (int32_t)(s->sleep_seconds / 3600 - before));
  } else
    s->energy = clamp(s->energy - (int32_t)hours);
  st_seal(s);
}
static void put32(uint8_t *p, uint32_t value) {
  for (unsigned i = 0; i < 4; ++i)
    p[i] = value >> (i * 8);
}
static uint32_t get32(const uint8_t *p) {
  uint32_t value = 0;
  for (unsigned i = 0; i < 4; ++i)
    value |= (uint32_t)p[i] << (i * 8);
  return value;
}
static uint32_t wire_hash(const uint8_t *p) {
  uint32_t h = 2166136261u;
  for (unsigned i = 0; i < 60; ++i) {
    h ^= p[i];
    h *= 16777619u;
  }
  return h;
}
void st_encode(const st_state_t *s, uint8_t data[ST_SAVE_BYTES]) {
  memset(data, 0, ST_SAVE_BYTES);
  put32(data, s->magic);
  put32(data + 4, s->version);
  data[8] = s->hunger;
  data[9] = s->mood;
  data[10] = s->energy;
  data[11] = s->clean;
  put32(data + 12, s->coins);
  put32(data + 16, s->joy_total);
  put32(data + 20, s->high_score);
  data[24] = s->owned_outfits;
  data[25] = s->visited_places;
  data[26] = s->outfit;
  data[27] = s->sleeping;
  put32(data + 28, s->active_seconds);
  put32(data + 32, s->sleep_seconds);
  data[36] = s->destination;
  put32(data + 60, wire_hash(data));
}
bool st_decode(st_state_t *s, const uint8_t *data, unsigned size) {
  if (!data || size != ST_SAVE_BYTES || get32(data + 60) != wire_hash(data))
    return false;
  st_state_t decoded;
  memset(&decoded, 0, sizeof(decoded));
  decoded.magic = get32(data);
  decoded.version = get32(data + 4);
  decoded.hunger = data[8];
  decoded.mood = data[9];
  decoded.energy = data[10];
  decoded.clean = data[11];
  decoded.coins = get32(data + 12);
  decoded.joy_total = get32(data + 16);
  decoded.high_score = get32(data + 20);
  decoded.owned_outfits = data[24];
  decoded.visited_places = data[25];
  decoded.outfit = data[26];
  decoded.sleeping = data[27];
  decoded.active_seconds = get32(data + 28);
  decoded.sleep_seconds = get32(data + 32);
  decoded.destination = data[36];
  st_seal(&decoded);
  if (!st_valid(&decoded))
    return false;
  *s = decoded;
  return true;
}
