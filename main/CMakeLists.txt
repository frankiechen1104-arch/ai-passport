idf_component_register(
    SRCS "main.c" "st_pet.c" "st_storage.c" "st_work.c" "st_ui.c" "st_controller.c"
         "../assets/images/st_otter/st_art.c" "../assets/fonts/st_font_16.c"
    INCLUDE_DIRS "." "../assets/images/st_otter"
    EMBED_FILES
         "../assets/images/st_otter/background_0.raw"
         "../assets/images/st_otter/background_1.raw"
         "../assets/images/st_otter/background_2.raw"
         "../assets/images/st_otter/background_3.raw"
         "../assets/images/st_otter/background_4.raw"
         "../assets/images/st_otter/background_5.raw"
         "../assets/images/st_otter/background_6.raw"
         "../assets/images/st_otter/background_7.raw"
         "../assets/images/st_otter/outfit_0.raw"
         "../assets/images/st_otter/outfit_1.raw"
         "../assets/images/st_otter/outfit_2.raw"
         "../assets/images/st_otter/outfit_3.raw"
         "../assets/images/st_otter/outfit_4.raw"
         "../assets/images/st_otter/outfit_5.raw"
    REQUIRES bsp esp_hw_support esp_timer nvs_flash
)
