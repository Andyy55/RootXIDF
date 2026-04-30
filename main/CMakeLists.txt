# Daftarin semua file .c lu sesuai foldernya
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
                       INCLUDE_DIRS "include" ".")

# JIMAT SAKTI BYPASS DEAUTH
target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,-zmuldefs")
