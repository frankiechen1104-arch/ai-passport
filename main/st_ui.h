#pragma once
#include "st_animation.h"
#include "st_pet.h"
#include "st_work.h"
typedef enum {
  PAGE_HOME,
  PAGE_FOOD,
  PAGE_TRAVEL,
  PAGE_WARDROBE,
  PAGE_COLLECTION,
  PAGE_SLEEP,
  PAGE_WORK,
  PAGE_RESULT
} st_page_t;
typedef struct {
  st_page_t page;
  unsigned selected, destination;
  const char *notice;
  bool save_error;
  int battery;
  uint32_t reward;
  st_motion_t motion;
  int64_t motion_started_ms;
  uint32_t motion_serial;
} st_view_t;
void st_ui_create(void);
void st_ui_render(const st_state_t *state, const st_view_t *view,
                  const st_work_t *work, int64_t now_ms);
