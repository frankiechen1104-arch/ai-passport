#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "bsp_pins.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "key_guard.h"
#include "nvs_flash.h"
#include "st_controller.h"
#include "st_pet.h"
#include "st_storage.h"
#include "st_ui.h"
#include "st_work.h"
#include <string.h>
static const char *TAG = "st_otter";
typedef st_input_t input_event_t;
_Static_assert((int)BSP_BTN_OK == (int)ST_BTN_OK &&
                   (int)BSP_BTN_UP == (int)ST_BTN_UP &&
                   (int)BSP_BTN_DOWN == (int)ST_BTN_DOWN,
               "Key mapping");
_Static_assert((int)BSP_BTN_PRESS == (int)ST_BTN_PRESS &&
                   (int)BSP_BTN_CLICK == (int)ST_BTN_CLICK &&
                   (int)BSP_BTN_LONG == (int)ST_BTN_LONG,
               "Event mapping");
static QueueHandle_t s_queue;
static bool s_input_ready;
static key_guard_t s_key_guard;
static st_controller_t s_app;
static bool s_storage_ready;
#define s_pet s_app.pet
#define s_view s_app.view
#define s_work s_app.work
#define s_dirty s_app.dirty
#define s_save_after s_app.save_after
/* Runs on shared esp_timer task: timestamp and queue only. */
static void on_key(bsp_btn_t btn, bsp_btn_ev_t event, void *user) {
  (void)user;
  if (key_guard_consume(&s_key_guard, event == BSP_BTN_RELEASE,
                        esp_timer_get_time()))
    return;
  if (!s_input_ready || !s_queue)
    return;
  input_event_t input = {(st_button_t)btn, (st_event_t)event,
                         esp_timer_get_time() / 1000};
  (void)xQueueSend(s_queue, &input, 0);
}
static void app_task(void *arg) {
  (void)arg;
  int64_t last_second = esp_timer_get_time() / 1000000, last_ui = 0,
          next_battery = 0, next_checkpoint = 300000;
  for (;;) {
    input_event_t e;
    BaseType_t received = xQueueReceive(s_queue, &e, pdMS_TO_TICKS(20));
    /* Sample the update cutoff before draining. A newly queued timestamp can
     * never be incorrectly expired by a later clock sample in this frame. */
    int64_t now = esp_timer_get_time() / 1000;
    if (received) {
      st_controller_input(&s_app, &e);
      /* Drain timestamped events before advancing the game clock. */
      unsigned drained = 0;
      while (drained++ < 31 && xQueueReceive(s_queue, &e, 0))
        st_controller_input(&s_app, &e);
    }
    st_controller_update(&s_app, now);
    int64_t second = now / 1000;
    if (second > last_second) {
      st_advance(&s_pet, second - last_second);
      last_second = second;
      /* Save session fractions every five minutes, not each frame. */
      if (now >= next_checkpoint) {
        st_controller_dirty(&s_app, now);
        next_checkpoint = now + 300000;
      }
    }
    if (now >= next_battery) {
      s_view.battery = bsp_battery_soc();
      next_battery = now + 30000;
    }
    if (s_dirty && now >= s_save_after) {
      esp_err_t err =
          s_storage_ready ? st_storage_save(&s_pet) : ESP_ERR_INVALID_STATE;
      s_view.save_error = err != ESP_OK;
      if (err == ESP_OK) {
        s_dirty = false;
        s_save_after = 0;
      } else {
        ESP_LOGW(TAG, "Save failed: %s", esp_err_to_name(err));
        s_save_after = now + 10000;
      }
    }
    if (now - last_ui >= (s_view.page == PAGE_WORK ? 20 : 100) &&
        bsp_lvgl_lock(100)) {
      st_ui_render(&s_pet, &s_view, &s_work, now);
      bsp_lvgl_unlock();
      last_ui = now;
    }
  }
}
void app_main(void) {
  ESP_LOGI(TAG, "ST otter v2; ESP32-C3; offline monotonic time only");
  uint32_t wake = esp_sleep_get_wakeup_causes();
  if (wake & (1u << ESP_SLEEP_WAKEUP_GPIO))
    key_guard_arm(&s_key_guard, esp_timer_get_time(), 10000000);
  esp_err_t err = nvs_flash_init();
  s_storage_ready = err == ESP_OK;
  /* Never erase another application's settings to recover initialization
   * errors. */
  if (!s_storage_ready)
    ESP_LOGE(TAG, "NVS unavailable: %s", esp_err_to_name(err));
  st_controller_init(&s_app);
  err = s_storage_ready ? st_storage_load(&s_pet, 0) : ESP_ERR_INVALID_STATE;
  s_view.save_error = err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND;
  if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    ESP_LOGW(TAG, "Load failed: %s", esp_err_to_name(err));
  if (bsp_display_init() != ESP_OK || !bsp_lvgl_init()) {
    ESP_LOGE(TAG, "Display initialization failed");
    return;
  }
  bsp_display_backlight(85);
  (void)bsp_battery_init();
  s_view.battery = -1;
  s_view.page = s_pet.sleeping ? PAGE_SLEEP : PAGE_HOME;
  s_view.destination = s_pet.destination;
  s_queue = xQueueCreate(32, sizeof(input_event_t));
  if (!s_queue) {
    ESP_LOGE(TAG, "Input queue allocation failed");
    return;
  }
  if (!bsp_lvgl_lock(1000)) {
    vQueueDelete(s_queue);
    s_queue = NULL;
    return;
  }
  st_ui_create();
  st_ui_render(&s_pet, &s_view, &s_work, esp_timer_get_time() / 1000);
  bsp_lvgl_unlock();
  err = bsp_button_init(on_key, NULL);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Buttons unavailable: %s", esp_err_to_name(err));
    return;
  }
  key_guard_sync_released(&s_key_guard,
                          bsp_button_read_mv() >= BSP_BTN_MV_RELEASED_MIN);
  if (xTaskCreate(app_task, "st_app", 6144, NULL, 4, NULL) != pdPASS) {
    ESP_LOGE(TAG, "App task allocation failed");
    return;
  }
  s_input_ready = true;
  ESP_LOGI(TAG, "Ready; free heap %lu",
           (unsigned long)esp_get_free_heap_size());
}
