#include "st_storage.h"
#include "nvs.h"
#include <string.h>
esp_err_t st_storage_load(st_state_t *s, int64_t now) {
  st_init(s, now);
  nvs_handle_t h;
  esp_err_t err = nvs_open("st_pet", NVS_READONLY, &h);
  if (err == ESP_ERR_NVS_NOT_FOUND)
    return err;
  if (err != ESP_OK)
    return err;
  uint8_t data[ST_SAVE_BYTES];
  size_t size = sizeof(data);
  err = nvs_get_blob(h, "state_v2", data, &size);
  nvs_close(h);
  if (err != ESP_OK)
    return err;
  return st_decode(s, data, size) ? ESP_OK : ESP_ERR_INVALID_CRC;
}
esp_err_t st_storage_save(st_state_t *s) {
  if (!st_valid(s))
    return ESP_ERR_INVALID_STATE;
  uint8_t data[ST_SAVE_BYTES];
  st_encode(s, data);
  nvs_handle_t h;
  esp_err_t err = nvs_open("st_pet", NVS_READWRITE, &h);
  if (err != ESP_OK)
    return err;
  err = nvs_set_blob(h, "state_v2", data, sizeof(data));
  if (err == ESP_OK)
    err = nvs_commit(h);
  nvs_close(h);
  return err;
}
