#pragma once
#include <stdint.h>
#include "gfx.hpp"
#include "uix.hpp"
/// @brief The display
extern uix::display ttgo_display;
#define TTGO_LCD_WIDTH 240
#define TTGO_LCD_HEIGHT 135
#define TTGO_LCD_DIM {TTGO_LCD_WIDTH, TTGO_LCD_HEIGHT}
#define TTGO_PIXEL gfx::rgb_pixel<16>
#define TTGO_PALETTE gfx::palette_none<TTGO_PIXEL>
/// @brief The screen type
using ttgo_screen_t = uix::screen_ex<gfx::bitmap<TTGO_PIXEL, TTGO_PALETTE>>;
/// @brief The screen's surface type
using ttgo_surface_t = typename ttgo_screen_t::control_surface_type;
/// @brief The screen native color enum
using ttgo_color_t = gfx::color<typename ttgo_screen_t::pixel_type>;
/// @brief The default screen
extern ttgo_screen_t ttgo_default_screen;

#define TTGO_BUTTON_0 0
#define TTGO_BUTTON_35 35

/// @brief TTGO initialization options
typedef enum {
    /// @brief No button multiplexing, only ttgo_on_pressed_changed
    TTGO_DEFAULT = 0,
    /// @brief Button 0 has ttgo_on_clicks
    TTGO_BUTTON_0_CLICKS = 1,
    /// @brief Button 0 has ttgo_on_long_click
    TTGO_BUTTON_0_LONG = 2,
    /// @brief Button 0 has ttgo_on_clicks and ttgo_on_long_click
    TTGO_BUTTON_0_ALL = TTGO_BUTTON_0_CLICKS | TTGO_BUTTON_0_LONG,
    /// @brief Button 35 has ttgo_on_clicks
    TTGO_BUTTON_35_CLICKS = 4,
    /// @brief Button 35 has ttgo_on_long_click
    TTGO_BUTTON_35_LONG = 8,
    /// @brief Button 35 has ttgo_on_clicks and ttgo_on_long_click
    TTGO_BUTTON_35_ALL = TTGO_BUTTON_35_CLICKS | TTGO_BUTTON_35_LONG,
    /// @brief All buttons have all events
    TTGO_BUTTON_ALL = TTGO_BUTTON_0_ALL | TTGO_BUTTON_35_ALL
} ttgo_options_t;

/// @brief Initializes the TTGO
/// @param options The initialization options
void ttgo_init(ttgo_options_t options = TTGO_DEFAULT);
/// @brief Call within the main application loop
void ttgo_update(void);
/// @brief Called when a button is pressed or depressed
/// @param gpio The button GPIO number
/// @param pressed True if pressed, otherwise false
void ttgo_on_pressed_changed(uint8_t gpio, bool pressed);
/// @brief Called when a button is clicked one or more times in succession
/// @param gpio The button GPIO number
/// @param clicks The number of clicks
void ttgo_on_clicks(uint8_t gpio, unsigned clicks);
/// @brief Called when a button is pressed for a long duration
/// @param gpio The button GPIO number
void ttgo_on_long_click(uint8_t gpio);
/// @brief Called when the LCD is enabled or disabled
/// @param enabled True if enabled, otherwise false
void ttgo_on_lcd_enabled_changed(bool enabled);
/// @brief Indicates whether a button is pressed
/// @param gpio The GPIO number to query
/// @return True if pressed, otherwise false
bool ttgo_pressed(uint8_t gpio);
/// @brief Sets the backlight brightness as an integer percentage
/// @param percent The percent to set
void ttgo_backlight(uint8_t percent);
/// @brief Indicates whether the LCD is enabled or not
/// @return True if enabled, otherwise false
bool ttgo_lcd_enabled(void);
/// @brief Enables or disables the LCD
/// @param value True to wake the display, false to sleep it
void ttgo_lcd_enable(bool value);
/// @brief Fades the display backlight gradually before sleeping the display
void ttgo_lcd_fade_to_sleep(void);
/// @brief Indicates the battery level as an integer percentage
/// @return A percentage indicating the battery charge
uint8_t ttgo_battery_level(void);
/// @brief Indicates the battery voltage in millivolts
/// @return A value indicating the battery voltage
uint16_t ttgo_battery_voltage(void);
