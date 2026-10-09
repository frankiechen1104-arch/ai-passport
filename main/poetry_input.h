#pragma once
#include "poetry_model.h"

typedef enum { POETRY_RAW_PRESS, POETRY_RAW_LONG, POETRY_RAW_RELEASE } poetry_raw_event_t;
typedef struct {
    bool down, held, blocked;
    poetry_key_t key;
    uint64_t started_ms;
} poetry_input_t;
/* BSP already debounces ADC samples. This adapter grades each press/release once,
 * including rapid double taps, and consumes keys first pressed during waiting. */
bool poetry_input_event(poetry_input_t *input, poetry_key_t key,
                        poetry_raw_event_t event, uint64_t ms, bool allowed,
                        poetry_press_t *press);
