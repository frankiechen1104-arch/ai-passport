#include "poetry_ui.h"
#include "poetry_assets.h"
#include "src/misc/lv_text_private.h"
#include "bsp_display_rounding.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LV_FONT_DECLARE(poetry_font_16);
/* Host-only capture memory. Firmware never links this full-screen buffer. */
static uint16_t s_frame[320][240], s_draw[240 * 40];
static unsigned s_flushes;

static void flush(lv_display_t *d, const lv_area_t *area, uint8_t *pixels)
{
    const uint16_t *p = (const uint16_t *)pixels;
    for (int y = area->y1; y <= area->y2; ++y) {
        int32_t x1, x2;
        bool visible = bsp_display_rounded_row_span(y, 240, 320, 30, &x1, &x2);
        for (int x = area->x1; x <= area->x2; ++x) {
            uint16_t value = *p++;
            s_frame[y][x] = visible && x >= x1 && x <= x2 ? value : 0;
        }
    }
    ++s_flushes;
    lv_display_flush_ready(d);
}
static void screenshot(const char *directory, const char *name)
{
    if (!directory) return;
    char path[512];
    snprintf(path, sizeof(path), "%s/%s.ppm", directory, name);
    FILE *f = fopen(path, "wb");
    assert(f);
    fprintf(f, "P6\n240 320\n255\n");
    for (unsigned y = 0; y < 320; ++y) for (unsigned x = 0; x < 240; ++x) {
        uint16_t v = s_frame[y][x];
        uint8_t rgb[3] = { ((v >> 11) & 31) * 255 / 31, ((v >> 5) & 63) * 255 / 63, (v & 31) * 255 / 31 };
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
}
static unsigned labels_fit(lv_obj_t *parent, bool waiting)
{
    unsigned visible_labels = 0;
    for (unsigned i = 0; i < lv_obj_get_child_count(parent); ++i) {
        lv_obj_t *o = lv_obj_get_child(parent, i);
        if (lv_obj_has_flag(o, LV_OBJ_FLAG_HIDDEN)) continue;
        if (!lv_obj_check_type(o, &lv_label_class)) continue;
        ++visible_labels;
        const char *text = lv_label_get_text(o);
        uint32_t index = 0;
        while (text[index]) {
            uint32_t cp = lv_text_encoded_next(text, &index);
            if (cp == '\n') continue;
            lv_font_glyph_dsc_t glyph = {0};
            assert(lv_font_get_glyph_dsc(&poetry_font_16, &glyph, cp, 0) && !glyph.is_placeholder);
        }
        if (waiting) assert(!strcmp(text, "正在提取无意义"));
        lv_point_t size;
        lv_text_get_size(&size, text, lv_obj_get_style_text_font(o, 0),
            lv_obj_get_style_text_letter_space(o, 0), lv_obj_get_style_text_line_space(o, 0),
            LV_COORD_MAX, LV_TEXT_FLAG_NONE);
        if (size.x > lv_obj_get_width(o) || size.y > lv_obj_get_height(o)) {
            fprintf(stderr, "Text overflow: %s (%d,%d) box (%d,%d)\n", text,
                (int)size.x, (int)size.y, (int)lv_obj_get_width(o), (int)lv_obj_get_height(o));
            abort();
        }
        assert(lv_obj_get_style_text_font(o, 0) == &poetry_font_16);
    }
    if (waiting) assert(visible_labels == 1);
    return visible_labels;
}

static void render(poetry_model_t *m, const char *notice, int battery)
{
    poetry_ui_render(m, notice, battery);
    lv_obj_update_layout(lv_screen_active());
    labels_fit(lv_screen_active(), m->page == POETRY_WAIT);
    lv_refr_now(NULL);
}

int main(int argc, char **argv)
{
    const char *directory = argc > 1 ? argv[1] : NULL;
    lv_init();
    lv_display_t *d = lv_display_create(240, 320);
    assert(d);
    lv_display_set_color_format(d, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(d, s_draw, NULL, sizeof(s_draw), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(d, flush);
    assert(poetry_ui_check_fonts());
    poetry_ui_create();
    poetry_model_t m;
    poetry_init(&m);
    for (unsigned p = 0; p < 4; ++p) {
        m.persona = p;
        m.page = POETRY_HOME;
        render(&m, "", -1);
        char name[32]; snprintf(name, sizeof(name), "home_%u", p);
        screenshot(directory, name);
    }
    m.page = POETRY_WAIT;
    for (unsigned t = 0; t < 2000; t += 25) {
        poetry_ui_animate(t);
        render(&m, "This must be hidden", 85);
        char name[32]; snprintf(name, sizeof(name), "wait_%04u", t);
        screenshot(directory, name);
    }
    /* All 1024 combinations exercise actual LVGL font lookup, line metrics,
     * drawing and the same small pool. Test page lifetime without rebuilding UI. */
    for (unsigned p = 0; p < 4; ++p) for (unsigned rarity = 0; rarity < 4; ++rarity) {
        for (unsigned v = 0; v < 64; ++v) {
            m.page = POETRY_RESULT;
            m.current = (poetry_record_t){ .id = 1, .persona = p, .rarity = rarity, .variant = v };
            render(&m, "", 99);
        }
        char name[32]; snprintf(name, sizeof(name), "poem_%u_%u", p, rarity);
        screenshot(directory, name);
    }
    m.page = POETRY_HISTORY; render(&m, "", 50); screenshot(directory, "history_empty");
    m.page = POETRY_BASKET; render(&m, "", 50); screenshot(directory, "basket_empty");
    const char *notices[] = { "存储不可用，未保存", "按键不可用，请重启",
        "废纸篓满了，请先取消收藏", "存储失败，收藏未更改", "已保存", "存储不可用，记录未保存" };
    for (unsigned i = 0; i < sizeof(notices) / sizeof(notices[0]); ++i) render(&m, notices[i], -1);
    /* Stress navigation and decoding of three layered images, without churn. */
    for (unsigned i = 0; i < 300; ++i) {
        m.page = (poetry_page_t)(i % 5); render(&m, "", 42);
        lv_tick_inc(25); lv_timer_handler();
    }
    lv_mem_monitor_t monitor;
    lv_mem_monitor(&monitor);
    printf("Poetry LVGL: PASS (1024 poems, one-label waiting, 80 frames, font/box coverage, page stress); pool peak=%u free=%u flushes=%u\n",
        (unsigned)monitor.max_used, (unsigned)monitor.free_size, s_flushes);
    assert(monitor.free_size > 4000);
    return 0;
}
