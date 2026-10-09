#include "poetry_model.h"
#include <string.h>

void poetry_init(poetry_model_t *m)
{
    memset(m, 0, sizeof(*m));
    m->next_id = 1;
}

poetry_rarity_t poetry_rarity_for_ticket(unsigned ticket)
{
    ticket %= 100;
    return ticket < 65 ? POETRY_COMMON : ticket < 90 ? POETRY_GOOD :
           ticket < 99 ? POETRY_ACCIDENT : POETRY_UNIVERSE;
}

const poetry_record_t *poetry_visible(const poetry_model_t *m)
{
    if (m->page == POETRY_RESULT) return &m->current;
    if (m->page == POETRY_HISTORY && m->cursor < m->history_count) return &m->history[m->cursor];
    if (m->page == POETRY_BASKET && m->cursor < m->favorite_count) return &m->favorites[m->cursor];
    return NULL;
}

bool poetry_is_favorite(const poetry_model_t *m, uint32_t id)
{
    for (unsigned i = 0; i < m->favorite_count; ++i) {
        if (m->favorites[i].id == id) return true;
    }
    return false;
}

static poetry_action_t toggle_favorite(poetry_model_t *m)
{
    const poetry_record_t *r = poetry_visible(m);
    if (!r) return POETRY_NO_CHANGE;
    for (unsigned i = 0; i < m->favorite_count; ++i) {
        if (m->favorites[i].id != r->id) continue;
        memmove(&m->favorites[i], &m->favorites[i + 1],
                (m->favorite_count - i - 1) * sizeof(*r));
        --m->favorite_count;
        if (m->page == POETRY_BASKET && m->cursor >= m->favorite_count && m->cursor) --m->cursor;
        return POETRY_SAVE;
    }
    if (m->favorite_count == POETRY_FAVORITES_CAP) return POETRY_FULL;
    poetry_record_t copy = *r;
    memmove(&m->favorites[1], &m->favorites[0], m->favorite_count * sizeof(copy));
    m->favorites[0] = copy;
    ++m->favorite_count;
    return POETRY_SAVE;
}

poetry_action_t poetry_input(poetry_model_t *m, poetry_key_t key,
                             poetry_press_t press, uint64_t now,
                             unsigned ticket, uint32_t variant)
{
    if (key > POETRY_OK || press > POETRY_LONG || m->page == POETRY_WAIT) return POETRY_NO_CHANGE;
    if (press == POETRY_LONG) {
        m->cursor = 0;
        m->page = key == POETRY_UP ? POETRY_HISTORY : key == POETRY_DOWN ? POETRY_BASKET : POETRY_HOME;
        return POETRY_REDRAW;
    }
    if (m->page == POETRY_HOME) {
        if (key != POETRY_OK) {
            m->persona = (m->persona + (key == POETRY_UP ? 3 : 1)) % 4;
            return POETRY_REDRAW;
        }
        /* Never reuse an ID on overflow: bounded store IDs can safely be
         * renumbered, including matching favorites. This is centuries away. */
        if (m->next_id == UINT32_MAX) {
            uint32_t old_favorites[POETRY_FAVORITES_CAP];
            for (unsigned i = 0; i < m->favorite_count; ++i) {
                old_favorites[i] = m->favorites[i].id;
                m->favorites[i].id = i + 1;
            }
            for (unsigned i = 0; i < m->history_count; ++i) {
                uint32_t old = m->history[i].id;
                m->history[i].id = POETRY_FAVORITES_CAP + i + 1;
                for (unsigned j = 0; j < m->favorite_count; ++j) {
                    if (old == old_favorites[j]) m->history[i].id = m->favorites[j].id;
                }
            }
            m->next_id = POETRY_FAVORITES_CAP + POETRY_HISTORY_CAP + 1;
        }
        m->current = (poetry_record_t){ .id = m->next_id++, .variant = variant,
            .persona = m->persona, .rarity = poetry_rarity_for_ticket(ticket) };
        m->wait_started_ms = now;
        m->page = POETRY_WAIT;
        return POETRY_REDRAW;
    }
    if (key == POETRY_OK) return toggle_favorite(m);
    if (m->page == POETRY_RESULT) {
        m->page = POETRY_HOME;
        m->persona = (m->persona + (key == POETRY_UP ? 3 : 1)) % 4;
    } else {
        unsigned count = m->page == POETRY_HISTORY ? m->history_count : m->favorite_count;
        if (key == POETRY_UP && m->cursor) --m->cursor;
        if (key == POETRY_DOWN && m->cursor + 1u < count) ++m->cursor;
    }
    return POETRY_REDRAW;
}

poetry_action_t poetry_tick(poetry_model_t *m, uint64_t now)
{
    if (m->page != POETRY_WAIT || now < m->wait_started_ms ||
        now - m->wait_started_ms < POETRY_WAIT_MS) return POETRY_NO_CHANGE;
    unsigned keep = m->history_count < POETRY_HISTORY_CAP ? m->history_count : POETRY_HISTORY_CAP - 1;
    memmove(&m->history[1], &m->history[0], keep * sizeof(m->current));
    m->history[0] = m->current;
    m->history_count = keep + 1;
    m->page = POETRY_RESULT;
    return POETRY_SAVE;
}

unsigned poetry_lines(const poetry_record_t *r, const char *lines[4])
{
    if (!r || r->persona > 3 || r->rarity > 3) return 0;
    const poetry_bank_t *b = &poetry_corpus[r->persona][r->rarity];
    unsigned n = 0;
    lines[n++] = b->opening[r->variant & 3];
    if (r->variant & 16) lines[n++] = b->middle[0];
    if (r->variant & 32) lines[n++] = b->middle[1];
    lines[n++] = b->ending[(r->variant >> 2) & 3];
    return n;
}

static void put32(uint8_t *b, uint32_t v)
{
    for (unsigned i = 0; i < 4; ++i) b[i] = (uint8_t)(v >> (i * 8));
}
static uint32_t get32(const uint8_t *b)
{
    uint32_t v = 0;
    for (unsigned i = 0; i < 4; ++i) v |= (uint32_t)b[i] << (i * 8);
    return v;
}
static uint32_t crc32(const uint8_t *b, size_t n)
{
    uint32_t crc = UINT32_MAX;
    for (size_t i = 0; i < n; ++i) {
        crc ^= b[i];
        for (unsigned j = 0; j < 8; ++j) crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}
static bool valid(const poetry_model_t *m)
{
    if (m->persona > 3 || m->history_count > POETRY_HISTORY_CAP ||
        m->favorite_count > POETRY_FAVORITES_CAP || !m->next_id) return false;
    for (unsigned list = 0; list < 2; ++list) {
        const poetry_record_t *rs = list ? m->favorites : m->history;
        unsigned n = list ? m->favorite_count : m->history_count;
        for (unsigned i = 0; i < n; ++i) {
            if (!rs[i].id || rs[i].id >= m->next_id || rs[i].persona > 3 || rs[i].rarity > 3) return false;
            for (unsigned j = 0; j < i; ++j) if (rs[j].id == rs[i].id) return false;
        }
    }
    for (unsigned i = 0; i < m->history_count; ++i) {
        for (unsigned j = 0; j < m->favorite_count; ++j) {
            const poetry_record_t *a = &m->history[i], *b = &m->favorites[j];
            if (a->id == b->id && (a->variant != b->variant || a->persona != b->persona || a->rarity != b->rarity)) return false;
        }
    }
    return true;
}
bool poetry_encode(const poetry_model_t *m, uint8_t *b, size_t n)
{
    if (!m || !b || n != POETRY_STORE_SIZE || !valid(m)) return false;
    memset(b, 0, n);
    memcpy(b, "UPV1", 4);
    put32(b + 4, POETRY_CORPUS_VERSION);
    put32(b + 8, m->next_id);
    b[12] = m->persona; b[13] = m->history_count; b[14] = m->favorite_count;
    for (unsigned list = 0; list < 2; ++list) {
        const poetry_record_t *rs = list ? m->favorites : m->history;
        unsigned count = list ? m->favorite_count : m->history_count;
        size_t offset = 20 + (list ? POETRY_HISTORY_CAP * 12 : 0);
        for (unsigned i = 0; i < count; ++i) {
            uint8_t *p = b + offset + i * 12;
            put32(p, rs[i].id); put32(p + 4, rs[i].variant);
            p[8] = rs[i].persona; p[9] = rs[i].rarity;
        }
    }
    put32(b + n - 4, crc32(b, n - 4));
    return true;
}
bool poetry_decode(poetry_model_t *m, const uint8_t *b, size_t n)
{
    if (!m || !b || n != POETRY_STORE_SIZE || memcmp(b, "UPV1", 4) ||
        get32(b + 4) != POETRY_CORPUS_VERSION || get32(b + n - 4) != crc32(b, n - 4) ||
        b[13] > POETRY_HISTORY_CAP || b[14] > POETRY_FAVORITES_CAP) return false;
    poetry_model_t decoded;
    poetry_init(&decoded);
    decoded.next_id = get32(b + 8);
    decoded.persona = b[12]; decoded.history_count = b[13]; decoded.favorite_count = b[14];
    for (unsigned list = 0; list < 2; ++list) {
        poetry_record_t *rs = list ? decoded.favorites : decoded.history;
        unsigned count = list ? decoded.favorite_count : decoded.history_count;
        size_t offset = 20 + (list ? POETRY_HISTORY_CAP * 12 : 0);
        for (unsigned i = 0; i < count; ++i) {
            const uint8_t *p = b + offset + i * 12;
            rs[i] = (poetry_record_t){ .id = get32(p), .variant = get32(p + 4), .persona = p[8], .rarity = p[9] };
        }
    }
    if (!valid(&decoded)) return false;
    *m = decoded;
    return true;
}
int poetry_drift(uint32_t ms)
{
    unsigned t = ms % 8000;
    return (int)((t <= 4000 ? t : 8000 - t) * 12 / 4000) - 6;
}
int poetry_paddle(uint32_t ms)
{
    unsigned t = ms % 1600;
    return (int)((t <= 800 ? t : 1600 - t) * 4 / 800) - 2;
}
