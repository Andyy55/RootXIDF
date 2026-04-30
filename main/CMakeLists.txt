idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS ".")

# PAKSA COMPILER BUAT NERIMA FUNGSI BYPASSER PALSU KITA
target_link_libraries(${COMPONENT_LIB} "-Wl,-zmuldefs")
