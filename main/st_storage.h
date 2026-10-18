#ifndef ST_STORAGE_H
#define ST_STORAGE_H
#include "esp_err.h"
#include "st_pet.h"
esp_err_t st_storage_load(st_state_t *state, int64_t now);
esp_err_t st_storage_save(st_state_t *state);
#endif
