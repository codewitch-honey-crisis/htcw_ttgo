#pragma once
#include "gfx.hpp"
#include "uix.hpp"
extern uix::display ttgo_display;
#define TTGO_LCD_WIDTH 240
#define TTGO_LCD_HEIGHT 135
#define TTGO_LCD_DIM { TTGO_LCD_WIDTH, TTGO_LCD_HEIGHT }
#define TTGO_PIXEL gfx::rgb_pixel<16>
#define TTGO_PALETTE gfx::palette_none<TTGO_PIXEL>
using ttgo_screen_t = uix::screen_ex<gfx::bitmap<TTGO_PIXEL,TTGO_PALETTE>>;
using ttgo_surface_t = typename ttgo_screen_t::control_surface_type;
using ttgo_color_t = gfx::color<typename ttgo_screen_t::pixel_type>;
extern ttgo_screen_t ttgo_default_screen;

#define TTGO_BUTTON_0 0
#define TTGO_BUTTON_35 35
typedef enum {
    TTGO_DEFAULT = 0,
    TTGO_BUTTON_0_CLICKS = 1,
    TTGO_BUTTON_0_LONG = 2,
    TTGO_BUTTON_0_ALL = TTGO_BUTTON_0_CLICKS | TTGO_BUTTON_0_LONG,
    TTGO_BUTTON_35_CLICKS = 4,
    TTGO_BUTTON_35_LONG = 8,
    TTGO_BUTTON_35_ALL = TTGO_BUTTON_35_CLICKS | TTGO_BUTTON_35_LONG,
    TTGO_BUTTON_ALL = TTGO_BUTTON_0_ALL | TTGO_BUTTON_35_ALL 
} ttgo_options_t;
void ttgo_init(ttgo_options_t options = TTGO_DEFAULT);
void ttgo_update(void);
void ttgo_on_pressed_changed(uint8_t gpio,bool pressed);
void ttgo_on_clicks(uint8_t gpio,unsigned clicks);
void ttgo_on_long_click(uint8_t gpio);
void ttgo_on_lcd_enabled_changed(bool enabled);
bool ttgo_pressed(uint8_t gpio);
void ttgo_backlight(uint8_t percent);
bool ttgo_lcd_enabled(void);
void ttgo_lcd_enable(bool value);
void ttgo_lcd_fade_to_sleep(void);
uint8_t ttgo_battery_level(void);
void ttgo_power_off(void); // only works when on battery
