#include "poetry_model.h"
#include "poetry_input.h"
#include "poetry_ui.h"
#include "poetry_store.h"
#include "bsp_display.h"
#include "bsp_button.h"
#include "bsp_battery.h"
#include "bsp_pins.h"
#include "key_guard.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <stdatomic.h>

static const char *TAG = "useless_poetry";
static QueueHandle_t s_queue;
static key_guard_t s_boot_guard;
static atomic_bool s_ready, s_overflow;
static poetry_model_t s_model, s_before;
static poetry_store_t s_storage;

typedef struct { bsp_btn_t button; bsp_btn_ev_t event; uint64_t ms; } app_event_t;
static uint64_t now_ms(void) { return (uint64_t)esp_timer_get_time() / 1000; }

/* Shared esp_timer callback: bounded queue write only; no UI, Flash or I2C. */
static void on_key(bsp_btn_t key, bsp_btn_ev_t event, void *user)
{
    (void)user;
    if (key_guard_consume(&s_boot_guard, event == BSP_BTN_RELEASE, esp_timer_get_time())) return;
    if (!atomic_load(&s_ready)) return;
    if (event != BSP_BTN_PRESS && event != BSP_BTN_RELEASE && event != BSP_BTN_LONG) return;
    app_event_t e = { .button = key, .event = event, .ms = now_ms() };
    if (xQueueSend(s_queue, &e, 0) != pdTRUE) atomic_store(&s_overflow, true);
}

static unsigned rarity_ticket(void)
{
    uint32_t value;
    const uint32_t limit = UINT32_MAX - UINT32_MAX % 100;
    do { value = esp_random(); } while (value >= limit);
    return value % 100;
}

void app_main(void)
{
    poetry_init(&s_model);
    const char *notice = poetry_store_open(&s_storage, &s_model) ? "" : "存储不可用，未保存";
    if (bsp_display_init() != ESP_OK || !bsp_lvgl_init()) {
        ESP_LOGE(TAG, "Display initialization failed");
        poetry_store_close(&s_storage);
        return;
    }
    s_queue = xQueueCreate(24, sizeof(app_event_t));
    if (!s_queue) { ESP_LOGE(TAG, "Input queue allocation failed"); return; }
    key_guard_arm(&s_boot_guard, esp_timer_get_time(), 10000000);
    esp_err_t button_error = bsp_button_init(on_key, NULL);
    if (button_error != ESP_OK) notice = "按键不可用，请重启";
    else key_guard_sync_released(&s_boot_guard, bsp_button_read_mv() >= BSP_BTN_MV_RELEASED_MIN);
    int battery = bsp_battery_init() == ESP_OK ? bsp_battery_soc() : -1;
    if (!bsp_lvgl_lock(1000)) { ESP_LOGE(TAG, "UI lock failed"); return; }
    if (!poetry_ui_check_fonts()) {
        ESP_LOGE(TAG, "Font coverage check failed");
        bsp_lvgl_unlock();
        return;
    }
    poetry_ui_create();
    poetry_ui_render(&s_model, notice, battery);
    bsp_lvgl_unlock();
    bsp_display_backlight(65);
    atomic_store(&s_ready, true);
    ESP_LOGI(TAG, "USELESS POETRY ready; heap=%u largest=%u; corpus=v%u store=%u",
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
        (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
        POETRY_CORPUS_VERSION, POETRY_STORE_SIZE);

    poetry_input_t input = {0};
    uint64_t activity = now_ms(), battery_at = activity, notice_until = 0;
    bool dimmed = false, dark = false, input_recover = false, persona_dirty = false, pending_redraw = false;
    poetry_page_t rendered_page = POETRY_HOME;
    for (;;) {
        app_event_t e;
        bool received = xQueueReceive(s_queue, &e, pdMS_TO_TICKS(dark ? 500 : 25)) == pdTRUE;
        uint64_t now = now_ms();
        poetry_action_t action = POETRY_NO_CHANGE;
        if (atomic_exchange(&s_overflow, false)) {
            input = (poetry_input_t){0}; input_recover = true;
            ESP_LOGW(TAG, "Input queue overflow; waiting for all keys to release");
        }
        if (input_recover && bsp_button_read_mv() >= BSP_BTN_MV_RELEASED_MIN) {
            xQueueReset(s_queue); input_recover = false; received = false;
        }
        if (!received && input.down) {
            /* Grade holds with the monotonic worker clock too: button 4.2.0's
             * second-tap state does not send a LONG event. */
            e = (app_event_t){ .button = (bsp_btn_t)input.key, .event = BSP_BTN_LONG, .ms = now };
        }
        if ((received || input.down) && !input_recover) {
            /* A wake press restores light only. Timestamp grading prevents old
             * queued presses from becoming actions after the waiting deadline. */
            bool allowed = s_model.page != POETRY_WAIT && !dark;
            if (s_model.page == POETRY_RESULT && e.ms < s_model.wait_started_ms + POETRY_WAIT_MS) allowed = false;
            poetry_raw_event_t raw = e.event == BSP_BTN_PRESS ? POETRY_RAW_PRESS :
                e.event == BSP_BTN_LONG ? POETRY_RAW_LONG : POETRY_RAW_RELEASE;
            poetry_press_t press;
            bool fire = poetry_input_event(&input, (poetry_key_t)e.button, raw, e.ms, allowed, &press);
            if (e.event == BSP_BTN_PRESS) {
                activity = now;
                if (dimmed || dark) { bsp_display_backlight(65); dimmed = dark = false; }
            }
            if (fire) {
                s_before = s_model;
                uint8_t previous_persona = s_model.persona;
                action = poetry_input(&s_model, (poetry_key_t)e.button, press, now, rarity_ticket(), esp_random());
                persona_dirty |= previous_persona != s_model.persona;
                activity = now;
                notice = "";
                if (action == POETRY_FULL) { notice = "废纸篓满了，请先取消收藏"; notice_until = now + 3000; }
                if (action == POETRY_SAVE) {
                    /* Storage runs in this application task, outside LVGL lock.
                     * Favorites become visible only after acknowledged commit. */
                    if (!poetry_store_save(&s_storage, &s_model)) { s_model = s_before; notice = "存储失败，收藏未更改"; }
                    else notice = "已保存";
                    notice_until = now + 2500;
                }
            }
        }
        poetry_action_t tick = s_model.page == POETRY_WAIT && rendered_page == POETRY_WAIT
                              ? poetry_tick(&s_model, now) : POETRY_NO_CHANGE;
        if (tick != POETRY_NO_CHANGE) { action = tick; activity = now; }
        if (notice_until && now >= notice_until) { notice_until = 0; notice = ""; action = POETRY_REDRAW; }
        if (now - battery_at >= 30000 && s_model.page != POETRY_WAIT) {
            battery = bsp_battery_soc(); battery_at = now; action = POETRY_REDRAW;
            ESP_LOGI(TAG, "heap=%u largest=%u task_stack_free=%u",
                (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
                (unsigned)uxTaskGetStackHighWaterMark(NULL));
        }
        bool redraw = pending_redraw || action != POETRY_NO_CHANGE;
        if ((redraw || s_model.page == POETRY_WAIT) && bsp_lvgl_lock(100)) {
            /* Start the two seconds when the wait scene is actually installed,
             * not when a key was queued or while waiting for the UI lock. */
            if (redraw && s_model.page == POETRY_WAIT && rendered_page != POETRY_WAIT) {
                s_model.wait_started_ms = now_ms();
            }
            if (redraw) poetry_ui_render(&s_model, notice, battery);
            rendered_page = s_model.page;
            pending_redraw = false;
            if (s_model.page == POETRY_WAIT) poetry_ui_animate((uint32_t)(now_ms() - s_model.wait_started_ms));
            bsp_lvgl_unlock();
        } else if (redraw) pending_redraw = true;
        /* Reveal the poem at the deadline before any Flash write. History stays
         * readable in RAM if storage fails, with a visible persistence warning. */
        if (tick == POETRY_SAVE || (persona_dirty && s_model.page != POETRY_WAIT && now - activity >= 1000)) {
            bool saved = poetry_store_save(&s_storage, &s_model); persona_dirty = false;
            if (!saved) {
                notice = "存储不可用，记录未保存"; notice_until = now_ms() + 4000;
                if (bsp_lvgl_lock(100)) { poetry_ui_render(&s_model, notice, battery); bsp_lvgl_unlock(); }
            }
        }
        if (s_model.page != POETRY_WAIT && now - activity >= 60000 && !dark) {
            bsp_display_backlight(0); dark = dimmed = true;
        } else if (s_model.page != POETRY_WAIT && now - activity >= 30000 && !dimmed) {
            bsp_display_backlight(10); dimmed = true;
        }
    }
}
