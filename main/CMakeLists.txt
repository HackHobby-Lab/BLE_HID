set(srcs "mainHid.c" "esp_hid_gap.c")
set(include_dirs ".")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "${include_dirs}"
                       REQUIRES esp_hid  # Ensure the esp_hid component is required
                       PRIV_REQUIRES nvs_flash)
