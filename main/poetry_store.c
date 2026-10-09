#include "poetry_store.h"
#include "nvs_flash.h"

/* Single-owner scratch buffer; never allocate the snapshot on the shared timer
 * stack. Store functions are called serially by the application worker. */
static uint8_t s_bytes[POETRY_STORE_SIZE];

void poetry_store_close(poetry_store_t *store)
{
    if (store->ready) nvs_close(store->handle);
    store->ready = false;
}

bool poetry_store_open(poetry_store_t *store, poetry_model_t *model)
{
    store->ready = false;
    if (nvs_flash_init() != ESP_OK ||
        nvs_open("useless_poetry", NVS_READWRITE, &store->handle) != ESP_OK) return false;
    size_t size = sizeof(s_bytes);
    esp_err_t e = nvs_get_blob(store->handle, "records_v1", s_bytes, &size);
    if (e == ESP_ERR_NVS_NOT_FOUND) { store->ready = true; return true; }
    if (e != ESP_OK || !poetry_decode(model, s_bytes, size)) {
        nvs_close(store->handle);
        return false;
    }
    store->ready = true;
    return true;
}

bool poetry_store_save(poetry_store_t *store, const poetry_model_t *model)
{
    if (!store->ready || !poetry_encode(model, s_bytes, sizeof(s_bytes))) return false;
    esp_err_t e = nvs_set_blob(store->handle, "records_v1", s_bytes, sizeof(s_bytes));
    if (e == ESP_OK) e = nvs_commit(store->handle);
    if (e != ESP_OK) {
        /* The durable outcome of a failed commit is uncertain. Do not keep a
         * pending handle which might acknowledge a failed operation later. */
        nvs_close(store->handle);
        store->ready = nvs_open("useless_poetry", NVS_READWRITE, &store->handle) == ESP_OK;
        return false;
    }
    return true;
}
