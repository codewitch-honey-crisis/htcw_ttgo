#pragma once
#include <Arduino.h>
#include <tft_io.hpp>
#include <st7789.hpp>
#include <button.hpp>
#include <gfx.hpp>
#include <lcd_miser.hpp>
using lcd_bus_t = arduino::tft_spi_ex<VSPI, 5, 19, -1, 18, SPI_MODE0,true,135*240*2+8,2>;                        
using lcd_t = arduino::st7789<135,240,16,23,-1,lcd_bus_t,1,true,400,200>;
using color_t = gfx::color<typename lcd_t::pixel_type>;
extern lcd_t lcd;
using button_a_t = arduino::int_button<35,10,true>;
using button_b_t = arduino::int_button<0,10,true>;
extern button_a_t button_a_raw;
extern button_b_t button_b_raw;
extern arduino::multi_button button_a;
extern arduino::multi_button button_b;
using dimmer_t = arduino::lcd_miser<4>;
extern dimmer_t dimmer;
using namespace arduino;
using namespace gfx;
extern void ttgo_initialize();
extern void ttgo_update();
