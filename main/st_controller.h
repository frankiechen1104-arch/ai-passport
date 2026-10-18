#pragma once
#include "st_ui.h"
typedef enum { ST_BTN_UP = 0, ST_BTN_DOWN, ST_BTN_OK } st_button_t;
typedef enum {
  ST_BTN_PRESS = 0,
  ST_BTN_CLICK,
  ST_BTN_DOUBLE,
  ST_BTN_LONG,
  ST_BTN_RELEASE
} st_event_t;
typedef struct {
  st_button_t btn;
  st_event_t event;
  int64_t ms;
} st_input_t;
typedef struct {
  st_state_t pet;
  st_view_t view;
  st_work_t work;
  bool dirty, ignore_ok_click;
  int64_t notice_until, save_after;
  unsigned home_selection;
} st_controller_t;
void st_controller_init(st_controller_t *c);
void st_controller_input(st_controller_t *c, const st_input_t *event);
void st_controller_update(st_controller_t *c, int64_t now);
void st_controller_dirty(st_controller_t *c, int64_t now);
