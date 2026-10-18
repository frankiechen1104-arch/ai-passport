#include "nvs.h"
#include "st_storage.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t record[ST_SAVE_BYTES];
static esp_err_t open_error, get_error, set_error, commit_error;
static unsigned closed, committed, written;
static size_t stored_size = ST_SAVE_BYTES;
esp_err_t nvs_open(const char *name, nvs_open_mode_t mode, nvs_handle_t *h) {
  assert(!strcmp(name, "st_pet"));
  (void)mode;
  *h = 1;
  return open_error;
}
esp_err_t nvs_get_blob(nvs_handle_t h, const char *key, void *data,
                       size_t *size) {
  assert(h == 1 && !strcmp(key, "state_v2"));
  assert(*size == ST_SAVE_BYTES);
  memcpy(data, record, stored_size);
  *size = stored_size;
  return get_error;
}
esp_err_t nvs_set_blob(nvs_handle_t h, const char *key, const void *data,
                       size_t size) {
  assert(h == 1 && !strcmp(key, "state_v2") && size == ST_SAVE_BYTES);
  ++written;
  if (set_error == ESP_OK)
    memcpy(record, data, size);
  return set_error;
}
esp_err_t nvs_commit(nvs_handle_t h) {
  assert(h == 1);
  ++committed;
  return commit_error;
}
void nvs_close(nvs_handle_t h) {
  assert(h == 1);
  ++closed;
}
int main(void) {
  st_state_t s, loaded;
  st_init(&s, 0);
  s.coins = 100;
  st_seal(&s);
  assert(st_storage_save(&s) == ESP_OK && written == 1 && committed == 1 &&
         closed == 1);
  assert(st_storage_load(&loaded, 0) == ESP_OK && loaded.coins == 100 &&
         closed == 2);
  record[9] ^= 1;
  assert(st_storage_load(&loaded, 0) == ESP_ERR_INVALID_CRC &&
         loaded.coins == 0);
  record[9] ^= 1;
  stored_size = 63;
  assert(st_storage_load(&loaded, 0) == ESP_ERR_INVALID_CRC);
  stored_size = 64;
  open_error = ESP_ERR_NVS_NOT_FOUND;
  assert(st_storage_load(&loaded, 0) == open_error && loaded.coins == 0);
  open_error = ESP_FAIL;
  assert(st_storage_save(&s) == ESP_FAIL);
  open_error = ESP_OK;
  set_error = ESP_FAIL;
  unsigned previous = committed;
  assert(st_storage_save(&s) == ESP_FAIL && committed == previous);
  set_error = ESP_OK;
  commit_error = ESP_FAIL;
  assert(st_storage_save(&s) == ESP_FAIL);
  commit_error = ESP_OK;
  get_error = ESP_FAIL;
  assert(st_storage_load(&loaded, 0) == ESP_FAIL);
  s.hunger = -1;
  st_seal(&s);
  previous = written;
  assert(st_storage_save(&s) == ESP_ERR_INVALID_STATE && written == previous);
  puts("ST NVS read/write/commit failures and corrupt records: PASS");
}
