#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define POETRY_WAIT_MS 2000u
#define POETRY_HISTORY_CAP 32u
#define POETRY_FAVORITES_CAP 24u
#define POETRY_STORE_SIZE (24u + 12u * (POETRY_HISTORY_CAP + POETRY_FAVORITES_CAP))
#define POETRY_CORPUS_VERSION 1u

typedef enum { POETRY_ABSURD, POETRY_MELANCHOLY, POETRY_CORPORATE, POETRY_COSMIC } poetry_persona_t;
typedef enum { POETRY_COMMON, POETRY_GOOD, POETRY_ACCIDENT, POETRY_UNIVERSE } poetry_rarity_t;
typedef enum { POETRY_HOME, POETRY_WAIT, POETRY_RESULT, POETRY_HISTORY, POETRY_BASKET } poetry_page_t;
typedef enum { POETRY_UP, POETRY_DOWN, POETRY_OK } poetry_key_t;
typedef enum { POETRY_SHORT, POETRY_LONG } poetry_press_t;
typedef enum { POETRY_NO_CHANGE, POETRY_REDRAW, POETRY_SAVE, POETRY_FULL } poetry_action_t;

/* Stable compact records refer to corpus v1; favorites survive history eviction.
 * Persist explicit little-endian bytes, never the compiler's struct layout. */
typedef struct {
    uint32_t id;
    uint32_t variant;
    uint8_t persona;
    uint8_t rarity;
} poetry_record_t;

typedef struct {
    poetry_page_t page;
    uint8_t persona, cursor, history_count, favorite_count;
    uint32_t next_id;
    uint64_t wait_started_ms;
    poetry_record_t current;
    poetry_record_t history[POETRY_HISTORY_CAP];
    poetry_record_t favorites[POETRY_FAVORITES_CAP];
} poetry_model_t;

typedef struct {
    const char *opening[4];
    const char *middle[2];
    const char *ending[4];
} poetry_bank_t;

extern const poetry_bank_t poetry_corpus[4][4];
extern const char *const poetry_persona_names[4];
extern const char *const poetry_persona_english[4];
extern const char *const poetry_rarity_names[4];

void poetry_init(poetry_model_t *model);
poetry_rarity_t poetry_rarity_for_ticket(unsigned ticket);
/* Input RNG adapters reject values >= UINT32_MAX - UINT32_MAX % 100 first. */
poetry_action_t poetry_input(poetry_model_t *model, poetry_key_t key,
                             poetry_press_t press, uint64_t now_ms,
                             unsigned ticket, uint32_t variant);
poetry_action_t poetry_tick(poetry_model_t *model, uint64_t now_ms);
const poetry_record_t *poetry_visible(const poetry_model_t *model);
bool poetry_is_favorite(const poetry_model_t *model, uint32_t id);
unsigned poetry_lines(const poetry_record_t *record, const char *lines[4]);
/* Transactional decode: on corruption/version mismatch the destination is unchanged. */
bool poetry_encode(const poetry_model_t *model, uint8_t *bytes, size_t size);
bool poetry_decode(poetry_model_t *model, const uint8_t *bytes, size_t size);
/* Integer-only gentle triangle motion, no large bitmap or floating-point trig. */
int poetry_drift(uint32_t elapsed_ms);
int poetry_paddle(uint32_t elapsed_ms);
