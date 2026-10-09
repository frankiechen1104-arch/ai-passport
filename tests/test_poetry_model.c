#include "poetry_model.h"
#include "poetry_input.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void generate(poetry_model_t *m, unsigned ticket, uint32_t variant, uint64_t start)
{
    m->page = POETRY_HOME;
    assert(poetry_input(m, POETRY_OK, POETRY_SHORT, start, ticket, variant) == POETRY_REDRAW);
    assert(m->page == POETRY_WAIT);
    assert(poetry_tick(m, start + 1999) == POETRY_NO_CHANGE);
    assert(poetry_input(m, POETRY_DOWN, POETRY_LONG, start + 1999, 0, 0) == POETRY_NO_CHANGE);
    assert(poetry_tick(m, start + 2000) == POETRY_SAVE);
    assert(m->page == POETRY_RESULT);
    assert(poetry_tick(m, start + 2500) == POETRY_NO_CHANGE);
}

static void rarity_and_corpus(void)
{
    unsigned counts[4] = {0};
    for (unsigned t = 0; t < 100; ++t) ++counts[poetry_rarity_for_ticket(t)];
    assert(counts[0] == 65 && counts[1] == 25 && counts[2] == 9 && counts[3] == 1);
    unsigned totals[5] = {0};
    for (unsigned p = 0; p < 4; ++p) for (unsigned r = 0; r < 4; ++r) {
        for (uint32_t v = 0; v < 64; ++v) {
            poetry_record_t record = { .persona = p, .rarity = r, .variant = v };
            const char *lines[4];
            unsigned n = poetry_lines(&record, lines);
            assert(n >= 2 && n <= 4);
            ++totals[n];
            for (unsigned i = 0; i < n; ++i) {
                assert(lines[i] && strlen(lines[i]) > 0 && strlen(lines[i]) < 64);
                assert(!strchr(lines[i], '\n'));
            }
        }
    }
    assert(totals[2] == 256 && totals[3] == 512 && totals[4] == 256);
    poetry_record_t bad = { .persona = 4 };
    const char *lines[4];
    assert(poetry_lines(&bad, lines) == 0);
}

static void navigation_and_history(void)
{
    poetry_model_t m;
    poetry_init(&m);
    assert(m.page == POETRY_HOME && m.next_id == 1);
    assert(poetry_visible(&m) == NULL);
    assert(poetry_input(&m, POETRY_UP, POETRY_SHORT, 0, 0, 0) == POETRY_REDRAW && m.persona == 3);
    for (unsigned i = 0; i < 4; ++i) poetry_input(&m, POETRY_DOWN, POETRY_SHORT, 0, 0, 0);
    assert(m.persona == 3);
    poetry_input(&m, POETRY_UP, POETRY_LONG, 0, 0, 0);
    assert(m.page == POETRY_HISTORY && !poetry_visible(&m));
    assert(poetry_input(&m, POETRY_OK, POETRY_SHORT, 0, 0, 0) == POETRY_NO_CHANGE);
    poetry_input(&m, POETRY_DOWN, POETRY_LONG, 0, 0, 0);
    assert(m.page == POETRY_BASKET && !poetry_visible(&m));
    poetry_input(&m, POETRY_OK, POETRY_LONG, 0, 0, 0);
    generate(&m, 99, 63, 7000);
    assert(m.current.rarity == POETRY_UNIVERSE && m.current.persona == 3);
    assert(poetry_input(&m, POETRY_OK, POETRY_SHORT, 9000, 0, 0) == POETRY_SAVE);
    uint32_t favorite_id = m.current.id;
    for (unsigned i = 0; i < 70; ++i) generate(&m, i % 100, i, 10000 + i * 3000);
    assert(m.history_count == POETRY_HISTORY_CAP && m.history[0].id == 71 && m.history[31].id == 40);
    assert(poetry_is_favorite(&m, favorite_id) && m.favorites[0].variant == 63);
    poetry_input(&m, POETRY_UP, POETRY_LONG, 0, 0, 0);
    for (unsigned i = 0; i < 100; ++i) poetry_input(&m, POETRY_DOWN, POETRY_SHORT, 0, 0, 0);
    assert(m.cursor == POETRY_HISTORY_CAP - 1);
    assert(poetry_input(&m, POETRY_OK, POETRY_SHORT, 0, 0, 0) == POETRY_SAVE);
    unsigned cursor = m.cursor;
    poetry_input(&m, POETRY_OK, POETRY_SHORT, 0, 0, 0);
    assert(m.cursor == cursor); /* Removing a favorite must not move history. */
    poetry_input(&m, POETRY_DOWN, POETRY_LONG, 0, 0, 0);
    assert(poetry_visible(&m)->id == favorite_id);
    poetry_input(&m, POETRY_OK, POETRY_SHORT, 0, 0, 0);
    assert(m.favorite_count == 0 && m.cursor == 0 && !poetry_visible(&m));
}

static void favorite_capacity(void)
{
    poetry_model_t m;
    poetry_init(&m);
    for (unsigned i = 0; i < POETRY_FAVORITES_CAP; ++i) {
        generate(&m, i, i, i * 3000);
        assert(poetry_input(&m, POETRY_OK, POETRY_SHORT, 0, 0, 0) == POETRY_SAVE);
    }
    poetry_record_t kept = m.favorites[POETRY_FAVORITES_CAP - 1];
    generate(&m, 99, 63, 100000);
    assert(poetry_input(&m, POETRY_OK, POETRY_SHORT, 0, 0, 0) == POETRY_FULL);
    assert(m.favorite_count == POETRY_FAVORITES_CAP && m.favorites[23].id == kept.id);
    poetry_input(&m, POETRY_DOWN, POETRY_LONG, 0, 0, 0);
    m.cursor = 23;
    poetry_input(&m, POETRY_OK, POETRY_SHORT, 0, 0, 0);
    assert(m.cursor == 22 && m.favorite_count == 23);
}

static void persistence(void)
{
    poetry_model_t m, loaded, unchanged;
    uint8_t bytes[POETRY_STORE_SIZE];
    poetry_init(&m);
    for (unsigned i = 0; i < 40; ++i) {
        m.persona = i % 4;
        generate(&m, i * 7 % 100, i * 9, i * 3000);
        if (i % 3 == 0) poetry_input(&m, POETRY_OK, POETRY_SHORT, 0, 0, 0);
    }
    assert(poetry_encode(&m, bytes, sizeof(bytes)));
    assert(poetry_decode(&loaded, bytes, sizeof(bytes)));
    assert(loaded.page == POETRY_HOME && loaded.persona == m.persona && loaded.next_id == m.next_id);
    assert(loaded.history_count == 32 && loaded.favorite_count == m.favorite_count);
    for (unsigned i = 0; i < loaded.history_count; ++i) {
        assert(loaded.history[i].id == m.history[i].id && loaded.history[i].variant == m.history[i].variant);
        const char *a[4], *b[4];
        unsigned n = poetry_lines(&loaded.history[i], a);
        assert(n == poetry_lines(&m.history[i], b));
        for (unsigned j = 0; j < n; ++j) assert(!strcmp(a[j], b[j]));
    }
    unchanged = loaded;
    for (unsigned i = 0; i < sizeof(bytes); ++i) {
        bytes[i] ^= 0x80;
        assert(!poetry_decode(&loaded, bytes, sizeof(bytes)));
        assert(!memcmp(&loaded, &unchanged, sizeof(loaded)));
        bytes[i] ^= 0x80;
    }
    assert(!poetry_decode(&loaded, bytes, sizeof(bytes) - 1));
    assert(!poetry_encode(&m, bytes, sizeof(bytes) - 1));
    m.persona = 4;
    assert(!poetry_encode(&m, bytes, sizeof(bytes)));
    m.persona = 0;
    m.next_id = UINT32_MAX;
    generate(&m, 99, 63, 900000);
    assert(poetry_encode(&m, bytes, sizeof(bytes))); /* Overflow keeps favorites consistent. */
    assert(poetry_decode(&loaded, bytes, sizeof(bytes)));
}

static void input_grading(void)
{
    poetry_input_t i = {0};
    poetry_press_t press = POETRY_LONG;
    assert(!poetry_input_event(&i, POETRY_OK, POETRY_RAW_PRESS, 100, true, &press));
    assert(!poetry_input_event(&i, POETRY_OK, POETRY_RAW_PRESS, 101, true, &press));
    assert(!poetry_input_event(&i, POETRY_OK, POETRY_RAW_RELEASE, 120, true, &press));
    assert(!poetry_input_event(&i, POETRY_OK, POETRY_RAW_RELEASE, 140, true, &press));
    for (unsigned t = 0; t < 2; ++t) {
        assert(!poetry_input_event(&i, POETRY_OK, POETRY_RAW_PRESS, 200 + t * 100, true, &press));
        assert(poetry_input_event(&i, POETRY_OK, POETRY_RAW_RELEASE, 250 + t * 100, true, &press));
        assert(press == POETRY_SHORT);
    }
    poetry_input_event(&i, POETRY_UP, POETRY_RAW_PRESS, 1000, true, &press);
    assert(!poetry_input_event(&i, POETRY_UP, POETRY_RAW_LONG, 1499, true, &press));
    assert(poetry_input_event(&i, POETRY_UP, POETRY_RAW_LONG, 1500, true, &press) && press == POETRY_LONG);
    assert(!poetry_input_event(&i, POETRY_UP, POETRY_RAW_LONG, 1600, true, &press));
    assert(!poetry_input_event(&i, POETRY_UP, POETRY_RAW_RELEASE, 1700, true, &press));
    poetry_input_event(&i, POETRY_DOWN, POETRY_RAW_PRESS, 2000, false, &press);
    assert(!poetry_input_event(&i, POETRY_DOWN, POETRY_RAW_LONG, 2500, true, &press));
    assert(!poetry_input_event(&i, POETRY_DOWN, POETRY_RAW_RELEASE, 2800, true, &press));
    poetry_input_event(&i, POETRY_DOWN, POETRY_RAW_PRESS, 3000, true, &press);
    assert(!poetry_input_event(&i, POETRY_UP, POETRY_RAW_RELEASE, 3100, true, &press));
    assert(poetry_input_event(&i, POETRY_DOWN, POETRY_RAW_RELEASE, 3200, true, &press));
    /* A second rapid press may get no BSP LONG callback: the worker grades a
     * synthetic monotonic hold; a delayed release must still be a long action. */
    poetry_input_event(&i, POETRY_OK, POETRY_RAW_PRESS, 4000, true, &press);
    assert(poetry_input_event(&i, POETRY_OK, POETRY_RAW_RELEASE, 4700, true, &press));
    assert(press == POETRY_LONG);
    poetry_input_event(&i, POETRY_OK, POETRY_RAW_PRESS, 4800, true, &press);
    assert(poetry_input_event(&i, POETRY_OK, POETRY_RAW_LONG, 5300, true, &press));
    assert(!poetry_input_event(&i, POETRY_OK, POETRY_RAW_RELEASE, 5500, true, &press));
}

static void animation_bounds(void)
{
    for (unsigned t = 0; t <= 16000; t += 25) {
        assert(poetry_drift(t) >= -6 && poetry_drift(t) <= 6);
        assert(poetry_paddle(t) >= -2 && poetry_paddle(t) <= 2);
        if (t) {
            int d = poetry_drift(t) - poetry_drift(t - 25);
            int p = poetry_paddle(t) - poetry_paddle(t - 25);
            assert(d >= -1 && d <= 1 && p >= -1 && p <= 1);
        }
    }
    poetry_model_t m;
    poetry_init(&m);
    poetry_input(&m, POETRY_OK, POETRY_SHORT, UINT64_C(5000000000), 0, 0);
    assert(poetry_tick(&m, UINT64_C(4999999999)) == POETRY_NO_CHANGE);
    assert(poetry_tick(&m, UINT64_C(5000002000)) == POETRY_SAVE);
}

int main(void)
{
    rarity_and_corpus(); navigation_and_history(); favorite_capacity();
    persistence(); input_grading(); animation_bounds();
    puts("Poetry model: PASS (1024 poems, exact rarity tickets, 2000ms deadline, input, history, favorites, corruption)");
    return 0;
}
