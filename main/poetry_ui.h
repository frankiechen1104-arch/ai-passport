#pragma once
#include "poetry_model.h"
#include "lvgl.h"

/* Called only with BSP LVGL lock held (or by LVGL-owned code). No timers/tasks
 * are owned by the UI. Persistent object tree avoids churn and dangling callbacks. */
void poetry_ui_create(void);
void poetry_ui_render(const poetry_model_t *model, const char *notice, int battery);
void poetry_ui_animate(uint32_t elapsed_ms);
bool poetry_ui_check_fonts(void);
