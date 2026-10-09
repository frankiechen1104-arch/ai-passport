#pragma once
#include "poetry_model.h"
#include "nvs.h"

typedef struct {
    nvs_handle_t handle;
    bool ready;
} poetry_store_t;
/* Application worker context only: these operations can block on internal Flash.
 * This namespace owns no other application's data and never erases NVS. */
bool poetry_store_open(poetry_store_t *store, poetry_model_t *model);
bool poetry_store_save(poetry_store_t *store, const poetry_model_t *model);
void poetry_store_close(poetry_store_t *store);
