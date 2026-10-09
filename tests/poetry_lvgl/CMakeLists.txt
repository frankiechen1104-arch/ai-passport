cmake_minimum_required(VERSION 3.16)
project(poetry_lvgl_test C)
set(REPO "${CMAKE_CURRENT_LIST_DIR}/../..")
set(LV_BUILD_CONF_PATH "${CMAKE_CURRENT_LIST_DIR}/lv_conf.h" CACHE PATH "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "" FORCE)
add_subdirectory("${REPO}/managed_components/lvgl__lvgl" lvgl)
add_executable(test_poetry_ui
    test_poetry_ui.c
    "${REPO}/main/poetry_ui.c"
    "${REPO}/main/poetry_model.c"
    "${REPO}/main/poetry_corpus.c"
    "${REPO}/assets/fonts/poetry_font_16.c"
    "${REPO}/assets/images/poetry_assets.c"
    "${REPO}/components/bsp/src/bsp_display_rounding.c")
target_include_directories(test_poetry_ui PRIVATE "${REPO}/main" "${REPO}/components/bsp/src")
target_compile_options(test_poetry_ui PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_link_libraries(test_poetry_ui PRIVATE lvgl m)
