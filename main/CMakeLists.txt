idf_component_register(
    SRCS "poetry_app.c" "poetry_model.c" "poetry_corpus.c"
         "poetry_input.c" "poetry_ui.c" "poetry_store.c"
         "../assets/fonts/poetry_font_16.c"
         "../assets/images/poetry_assets.c"
    INCLUDE_DIRS "."
    REQUIRES bsp esp_hw_support esp_timer nvs_flash
)
