idf_component_register(SRCS "RootX.c"
                            "OLED/i2c.c"
                            "OLED/ssd1306_i2c.c"
                            "OLED/fonts.c"
                            "OLED/font_glcd_5x7.c"
                            "OLED/font_tahoma_8pt.c"
                            "managers/boot_system.c"
                            "managers/display_system.c"
                            "managers/input_system.c"
                            "managers/wifi_system.c"
                            "managers/sd_system.c"
                       INCLUDE_DIRS "include" "."
                       REQUIRES esp_wifi nvs_flash esp_netif esp_event esp_timer driver esp_driver_gpio fatfs sdmmc vfs)

# JIMAT SAKTI BYPASS DEAUTH TETEP AMAN DI SINI
target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,-zmuldefs")
