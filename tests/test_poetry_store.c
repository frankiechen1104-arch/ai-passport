#include "poetry_store.h"
#include "nvs_flash.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint8_t durable[POETRY_STORE_SIZE], staged[POETRY_STORE_SIZE];
static bool exists, handle_open;
static unsigned closes, sets, commits;
static esp_err_t init_error, open_error, read_error, set_error, commit_error;

esp_err_t nvs_flash_init(void) { return init_error; }
esp_err_t nvs_open(const char *name, int mode, nvs_handle_t *handle)
{
    assert(!strcmp(name, "useless_poetry") && mode == NVS_READWRITE);
    if (open_error) return open_error;
    assert(!handle_open);
    handle_open = true; *handle = 17;
    return ESP_OK;
}
void nvs_close(nvs_handle_t handle)
{
    assert(handle == 17 && handle_open); handle_open = false; ++closes;
    memset(staged, 0, sizeof(staged));
}
esp_err_t nvs_get_blob(nvs_handle_t handle, const char *key, void *bytes, size_t *size)
{
    assert(handle == 17 && handle_open && !strcmp(key, "records_v1"));
    if (read_error) return read_error;
    if (!exists) return ESP_ERR_NVS_NOT_FOUND;
    assert(*size == sizeof(durable));
    memcpy(bytes, durable, sizeof(durable)); *size = sizeof(durable);
    return ESP_OK;
}
esp_err_t nvs_set_blob(nvs_handle_t handle, const char *key, const void *bytes, size_t size)
{
    assert(handle == 17 && handle_open && !strcmp(key, "records_v1") && size == sizeof(staged));
    ++sets;
    if (set_error) return set_error;
    memcpy(staged, bytes, size);
    return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t handle)
{
    assert(handle == 17 && handle_open); ++commits;
    if (commit_error) return commit_error;
    memcpy(durable, staged, sizeof(durable)); exists = true;
    return ESP_OK;
}
static void reset(void)
{
    exists = handle_open = false; closes = sets = commits = 0;
    init_error = open_error = read_error = set_error = commit_error = 0;
    memset(durable, 0, sizeof(durable));
}
int main(void)
{
    poetry_model_t m, loaded;
    poetry_store_t store = {0};
    reset(); poetry_init(&m);
    assert(poetry_store_open(&store, &m) && store.ready);
    assert(sets == 0); /* Empty boot must not create/erase unrelated NVS data. */
    poetry_input(&m, POETRY_OK, POETRY_SHORT, 0, 99, 63);
    poetry_tick(&m, 2000);
    poetry_input(&m, POETRY_OK, POETRY_SHORT, 2000, 0, 0);
    assert(poetry_store_save(&store, &m) && sets == 1 && commits == 1);
    poetry_store_close(&store);
    poetry_init(&loaded);
    assert(poetry_store_open(&store, &loaded));
    assert(loaded.history_count == 1 && loaded.favorite_count == 1);
    assert(loaded.favorites[0].rarity == POETRY_UNIVERSE && loaded.favorites[0].variant == 63);
    uint8_t old[POETRY_STORE_SIZE]; memcpy(old, durable, sizeof(old));
    set_error = ESP_FAIL;
    assert(!poetry_store_save(&store, &m) && store.ready && commits == 1);
    assert(!memcmp(old, durable, sizeof(old)));
    set_error = 0; commit_error = ESP_FAIL;
    assert(!poetry_store_save(&store, &m) && store.ready && commits == 2);
    assert(!memcmp(old, durable, sizeof(old)));
    commit_error = 0;
    assert(poetry_store_save(&store, &m));
    poetry_store_close(&store);
    durable[26] ^= 0x80;
    unsigned prior_sets = sets;
    assert(!poetry_store_open(&store, &loaded) && !store.ready && !handle_open);
    assert(!poetry_store_save(&store, &m) && sets == prior_sets);
    reset(); init_error = ESP_FAIL;
    assert(!poetry_store_open(&store, &loaded));
    reset(); open_error = ESP_FAIL;
    assert(!poetry_store_open(&store, &loaded));
    reset(); read_error = ESP_ERR_NVS_INVALID_LENGTH;
    assert(!poetry_store_open(&store, &loaded) && closes == 1 && sets == 0);
    reset(); assert(poetry_store_open(&store, &m));
    commit_error = ESP_FAIL; open_error = ESP_FAIL;
    assert(!poetry_store_save(&store, &m) && !store.ready && !handle_open);
    poetry_store_close(&store);
    puts("Poetry NVS adapter: PASS (restart, namespace isolation, corrupt/oversize blob, init/read/set/commit/reopen faults)");
    return 0;
}
