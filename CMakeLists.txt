cmake_minimum_required(VERSION 3.24)
idf_component_register(
    SRCS 
        "./src/source/ttgo.cpp"
    INCLUDE_DIRS "." "./src"
    REQUIRES freertos driver htcw_uix htcw_gfx htcw_data htcw_ml htcw_io htcw_bits esp_lcd htcw_ttgo_power
)
