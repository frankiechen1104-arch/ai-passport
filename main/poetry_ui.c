#include "poetry_ui.h"
#include "poetry_assets.h"
#include <stdio.h>

LV_FONT_DECLARE(poetry_font_16);
static lv_obj_t *s_screen, *s_heading, *s_battery, *s_english, *s_name;
static lv_obj_t *s_body, *s_rarity, *s_counter, *s_help, *s_notice;
static lv_obj_t *s_water, *s_otter, *s_foot, *s_ripple;
static lv_obj_t *s_marks[6];
static const uint32_t s_paper[4] = { 0xf7f1e5, 0xeef1f5, 0xf4f1e9, 0x17283b };
static const uint32_t s_ink[4] = { 0x594a3d, 0x4a5b72, 0x465b52, 0xe3e9ed };
static const uint32_t s_accent[4] = { 0xb37455, 0x748ea9, 0x6f8272, 0xa8bdd1 };

static lv_obj_t *label(int x, int y, int w, int h, lv_text_align_t align)
{
    lv_obj_t *o = lv_label_create(s_screen);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_text_font(o, &poetry_font_16, 0);
    lv_obj_set_style_text_align(o, align, 0);
    lv_label_set_long_mode(o, LV_LABEL_LONG_CLIP);
    return o;
}
static void hidden(lv_obj_t *o, bool hide)
{
    if (hide) lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(o, LV_OBJ_FLAG_HIDDEN);
}

void poetry_ui_create(void)
{
    s_screen = lv_obj_create(NULL);
    lv_obj_remove_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(s_screen, 0, 0);
    lv_obj_set_style_border_width(s_screen, 0, 0);
    s_heading = label(24, 20, 145, 22, LV_TEXT_ALIGN_LEFT);
    s_battery = label(176, 20, 44, 22, LV_TEXT_ALIGN_RIGHT);
    s_english = label(20, 61, 200, 24, LV_TEXT_ALIGN_CENTER);
    s_name = label(20, 88, 200, 24, LV_TEXT_ALIGN_CENTER);
    s_body = label(18, 115, 204, 136, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_style_text_line_space(s_body, 7, 0);
    s_rarity = label(20, 80, 200, 24, LV_TEXT_ALIGN_CENTER);
    s_counter = label(20, 54, 200, 24, LV_TEXT_ALIGN_CENTER);
    s_help = label(20, 254, 200, 44, LV_TEXT_ALIGN_CENTER);
    s_notice = label(20, 226, 200, 24, LV_TEXT_ALIGN_CENTER);
    s_water = lv_obj_create(s_screen);
    lv_obj_remove_style_all(s_water);
    lv_obj_set_pos(s_water, 28, 119);
    lv_obj_set_size(s_water, 184, 91);
    lv_obj_set_style_radius(s_water, 42, 0);
    lv_obj_set_style_bg_color(s_water, lv_color_hex(0xc9dddc), 0);
    lv_obj_set_style_bg_opa(s_water, LV_OPA_40, 0);
    lv_obj_remove_flag(s_water, LV_OBJ_FLAG_SCROLLABLE);
    s_ripple = lv_image_create(s_screen);
    lv_image_set_src(s_ripple, &poetry_ripple);
    s_otter = lv_image_create(s_screen);
    lv_image_set_src(s_otter, &poetry_otter);
    s_foot = lv_image_create(s_screen);
    lv_image_set_src(s_foot, &poetry_foot);
    for (unsigned i = 0; i < 6; ++i) {
        s_marks[i] = lv_obj_create(s_screen);
        lv_obj_remove_style_all(s_marks[i]);
        lv_obj_set_style_bg_opa(s_marks[i], LV_OPA_40, 0);
        lv_obj_remove_flag(s_marks[i], LV_OBJ_FLAG_SCROLLABLE);
    }
    poetry_ui_animate(0);
    lv_screen_load(s_screen);
}

void poetry_ui_animate(uint32_t ms)
{
    int drift = poetry_drift(ms);
    lv_obj_set_pos(s_otter, 43 + drift, 118);
    lv_obj_set_pos(s_foot, 158 + drift, 164 + poetry_paddle(ms));
    lv_obj_set_pos(s_ripple, 70 + drift, 184 + poetry_paddle(ms) / 2);
    lv_obj_set_style_opa(s_ripple, 85 + (poetry_paddle(ms) + 2) * 20, 0);
}

void poetry_ui_render(const poetry_model_t *m, const char *notice, int battery)
{
    bool wait = m->page == POETRY_WAIT, home = m->page == POETRY_HOME;
    const poetry_record_t *r = poetry_visible(m);
    unsigned p = r ? r->persona : m->persona;
    /* Distinct restrained motifs: crooked ink dashes, rain, ruled office paper,
     * and tiny constellations. All decoration disappears in the waiting scene. */
    for (unsigned i = 0; i < 6; ++i) {
        hidden(s_marks[i], wait);
        lv_obj_set_style_bg_color(s_marks[i], lv_color_hex(s_accent[p]), 0);
        lv_obj_set_style_radius(s_marks[i], p == POETRY_COSMIC ? 2 : 0, 0);
        if (p == POETRY_ABSURD) {
            lv_obj_set_pos(s_marks[i], i < 3 ? 21 + (int)i * 4 : 209 + (int)(i - 3) * 3, 54 + (int)(i % 3) * 4);
            lv_obj_set_size(s_marks[i], 4 + (int)(i % 3) * 2, 1);
        } else if (p == POETRY_MELANCHOLY) {
            lv_obj_set_pos(s_marks[i], i < 3 ? 23 + (int)i * 4 : 208 + (int)(i - 3) * 4, 53 + (int)(i % 3) * 3);
            lv_obj_set_size(s_marks[i], 1, 4 + (int)(i % 3));
        } else if (p == POETRY_CORPORATE) {
            lv_obj_set_pos(s_marks[i], 24, 47 + (int)i * 39);
            lv_obj_set_size(s_marks[i], 192, 1);
            lv_obj_set_style_bg_opa(s_marks[i], LV_OPA_10, 0);
        } else {
            lv_obj_set_pos(s_marks[i], i < 3 ? 23 + (int)i * 5 : 207 + (int)(i - 3) * 5, 53 + (int)(i % 3) * 4);
            lv_obj_set_size(s_marks[i], i % 2 ? 2 : 3, i % 2 ? 2 : 3);
        }
        if (p != POETRY_CORPORATE) lv_obj_set_style_bg_opa(s_marks[i], LV_OPA_40, 0);
    }
    /* The wait scene deliberately has exactly one text label. */
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(wait ? 0xf3f1e8 : s_paper[p]), 0);
    lv_obj_set_style_text_color(s_screen, lv_color_hex(wait ? 0x665d50 : s_ink[p]), 0);
    hidden(s_heading, wait); hidden(s_battery, wait);
    hidden(s_english, !home); hidden(s_name, !home);
    hidden(s_body, home || wait); hidden(s_rarity, home || wait || !r);
    hidden(s_counter, home || wait); hidden(s_help, wait);
    hidden(s_notice, wait || !notice || !*notice);
    hidden(s_water, !wait && !home); hidden(s_otter, !wait && !home);
    hidden(s_foot, !wait && !home); hidden(s_ripple, !wait && !home);
    lv_label_set_text(s_heading, "无用诗歌");
    char buf[320];
    if (battery < 0) lv_label_set_text(s_battery, "--%");
    else { snprintf(buf, sizeof(buf), "%d%%", battery); lv_label_set_text(s_battery, buf); }
    lv_label_set_text(s_english, poetry_persona_english[p]);
    lv_label_set_text(s_name, poetry_persona_names[p]);
    lv_obj_set_style_text_color(s_name, lv_color_hex(s_accent[p]), 0);
    lv_label_set_text(s_notice, notice ? notice : "");
    if (wait) {
        hidden(s_name, false);
        lv_label_set_text(s_name, "正在提取无意义");
        lv_obj_set_pos(s_name, 20, 225);
        lv_obj_set_style_text_color(s_name, lv_color_hex(0x665d50), 0);
        return;
    }
    lv_obj_set_pos(s_name, 20, 88);
    if (home) {
        lv_label_set_text(s_help, "上/下选人格  确定生成\n长上历史  长下废纸篓");
        poetry_ui_animate(1400); /* Static illustration when idle. */
        return;
    }
    if (r) {
        const char *lines[4];
        unsigned n = poetry_lines(r, lines);
        size_t used = 0;
        buf[0] = '\0';
        for (unsigned i = 0; i < n; ++i) {
            int written = snprintf(buf + used, sizeof(buf) - used, "%s%s", i ? "\n" : "", lines[i]);
            if (written < 0 || (size_t)written >= sizeof(buf) - used) break;
            used += (size_t)written;
        }
        lv_label_set_text(s_body, buf);
        lv_label_set_text(s_rarity, poetry_rarity_names[r->rarity]);
        /* Rarity also changes typography: star-like punctuation is supported by
         * the subset and no non-text badge steals room from the verse. */
        lv_obj_set_style_text_color(s_rarity, lv_color_hex(r->rarity >= POETRY_ACCIDENT ?
            (p == POETRY_COSMIC ? 0xe9c98e : 0x9b6847) : s_accent[p]), 0);
        if (m->page == POETRY_RESULT) {
            lv_label_set_text(s_help, poetry_is_favorite(m, r->id) ?
                "确定取消收藏\n上/下换人格  长确定返回" : "确定收藏\n上/下换人格  长确定返回");
        } else {
            lv_label_set_text(s_help, poetry_is_favorite(m, r->id) ?
                "确定取消收藏\n上/下翻页  长确定返回" : "确定收藏\n上/下翻页  长确定返回");
        }
    } else {
        lv_label_set_text(s_body, m->page == POETRY_BASKET ? "废纸篓还是空的\n好废话值得留下" : "还没有历史\n先生成一首无用诗");
        lv_label_set_text(s_help, "长确定返回");
    }
    unsigned count = m->page == POETRY_BASKET ? m->favorite_count : m->history_count;
    if (m->page == POETRY_RESULT) snprintf(buf, sizeof(buf), "%s", poetry_persona_names[p]);
    else snprintf(buf, sizeof(buf), "%s %u/%u", m->page == POETRY_BASKET ? "废纸篓" : "历史", count ? m->cursor + 1 : 0, count);
    lv_label_set_text(s_counter, buf);
}

static bool text_covered(const char *s)
{
    uint32_t i = 0;
    while (s[i]) {
        /* All content is validated UTF-8 from fixed local sources. Decode here
         * instead of depending on LVGL's private text iterator API. */
        uint8_t lead = (uint8_t)s[i++];
        uint32_t cp = lead;
        unsigned tails = lead < 0x80 ? 0 : lead < 0xe0 ? 1 : lead < 0xf0 ? 2 : 3;
        if (tails) cp = lead & (0x7f >> tails);
        for (unsigned t = 0; t < tails; ++t) {
            if (!s[i] || ((uint8_t)s[i] & 0xc0) != 0x80) return false;
            cp = (cp << 6) | ((uint8_t)s[i++] & 0x3f);
        }
        if (cp == '\n' || cp == '\r') continue;
        lv_font_glyph_dsc_t d = {0};
        if (!lv_font_get_glyph_dsc(&poetry_font_16, &d, cp, 0) || d.is_placeholder) return false;
    }
    return true;
}

bool poetry_ui_check_fonts(void)
{
    /* The inventory includes every UI message plus printable ASCII; host checks
     * regenerate it from sources so this runtime check cannot mask drift. */
    if (!text_covered(poetry_font_inventory)) return false;
    lv_font_glyph_dsc_t missing = {0};
    if (lv_font_get_glyph_dsc(&poetry_font_16, &missing, 0x9f98, 0) && !missing.is_placeholder) return false;
    for (unsigned p = 0; p < 4; ++p) for (unsigned r = 0; r < 4; ++r) {
        for (unsigned v = 0; v < 64; ++v) {
            poetry_record_t record = { .persona = p, .rarity = r, .variant = v };
            const char *lines[4];
            unsigned n = poetry_lines(&record, lines);
            for (unsigned i = 0; i < n; ++i) if (!text_covered(lines[i])) return false;
        }
    }
    return true;
}
