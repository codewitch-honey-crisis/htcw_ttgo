# htcw_ttgo

A little library for driving the TTGO's embedded screen, buttons, and power management from the ESP-IDF.

It already has htcw_uix/htcw_gfx UI and graphics wired up to it, plus button multiplexing support, battery indicators and screen sleep functionality.

## Example

```cpp
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include "ttgo.hpp"
#include "text_font_stream.hpp"
// import the gfx and uix namespaces since we'll be using them all over
using namespace gfx;
using namespace uix;
// UIX uses RGBA8888 so create a color enum for that:
using uix_color_t = color<uix_pixel>;
// the painter type for the button indicators:
using painter_t = painter<ttgo_surface_t>;
// the battery indicator type:
using battery_t = battery<ttgo_surface_t>;
// the text label type:
using label_t = label<ttgo_surface_t>;
// now create our truetype font with the text_font_stream
static tt_font text_font(text_font_stream, TTGO_LCD_HEIGHT / 5, font_size_units::px);
static mask_draw_cache draw_cache;
painter_t dot0, dot35;
battery_t batt;
static TickType_t text_fade_ts = 0;
static char text_data[64];
static font_measure_cache text_measure_cache;
static font_draw_cache text_draw_cache;
static uint8_t text_opacity = 255;
static label_t text;
static void start_text() {
    text.text(text_data);
    text_opacity = 255;
    text.color(uix_color_t::white);
    text.visible(true);
    text_fade_ts = xTaskGetTickCount();
}
void dot_on_paint(ttgo_surface_t& destination, const srect16& clip, void* state) {
    // uint8_t gpio = (int)state; // don't need this right now
    draw::aa_filled_rounded_rectangle(destination, destination.dimensions().bounds(), ttgo_color_t::white, 20, &draw_cache);
}
void ttgo_on_pressed_changed(uint8_t gpio, bool pressed) {
    ttgo_lcd_enable(true);
    if (pressed && (text_fade_ts != 0 || text_opacity != 255)) {
        text_fade_ts = 0;
        text_opacity = 255;
        text.invalidate();
    }
    if (gpio == TTGO_BUTTON_0) {
        dot0.visible(pressed);
    } else {
        dot35.visible(pressed);
    }
}
void ttgo_on_clicks(uint8_t gpio, unsigned clicks) {
    snprintf(text_data, sizeof(text_data), "%d clicks: %u", (int)gpio, clicks);
    start_text();
}
void ttgo_on_long_click(uint8_t gpio) {
    snprintf(text_data, sizeof(text_data), "%d long click", (int)gpio);
    start_text();
}
void ttgo_on_lcd_enabled_changed(bool enabled) {
    if (enabled) {
        batt.visible(true);
    }
}
static void loop_task(void* arg) {
    TickType_t wdt_ts = xTaskGetTickCount();
    int old_batt_level = -1;
    while (1) {
        // feed the watchdog timer
        if (xTaskGetTickCount() >= wdt_ts + pdMS_TO_TICKS(200)) {
            wdt_ts = xTaskGetTickCount();
            vTaskDelay(5);
        }
        // update the battery display
        uint8_t batt_level = ttgo_battery_level();
        if (batt_level != old_batt_level) {
            old_batt_level = batt_level;
            batt.level(batt_level);
        }
        if (text_fade_ts != 0) {
            if (xTaskGetTickCount() >= text_fade_ts + pdMS_TO_TICKS(10)) {
                text_fade_ts = xTaskGetTickCount();
                // fade the text
                text_opacity -= 2;
                // if it's done fading:
                if (text_opacity <= 1) {
                    // reset the text and fade
                    text.visible(false);
                    text_fade_ts = 0;
                    text_opacity = 255;
                    // sleep the display
                    ttgo_lcd_fade_to_sleep();
                } else {
                    text.color(uix_color_t::white.opacity8(text_opacity));
                }
            }
        }
        // must be called in app loop
        ttgo_update();
    }
}
extern "C" void app_main(void) {
    // initialize the TTGO with multiplexing on all buttons
    ttgo_init(TTGO_BUTTON_ALL);
    ttgo_lcd_rotation(2);
    printf("Battery voltage: %dmV\n",ttgo_battery_voltage());
    // preallocate our draw cache (not necessary, but slightly better performance)
    draw_cache.ensure(ttgo_default_screen.dimensions().width);
    // set up our font caches for faster rendering
    text_measure_cache.max_entries(40);
    text_measure_cache.initialize();
    text_draw_cache.max_entries(30);
    text_draw_cache.initialize();

    text_font.initialize();

    // set up the screen and controls
    ttgo_default_screen.background_color(ttgo_color_t::purple);

    // text label control
    text.bounds(srect16(0, 0, ttgo_default_screen.bounds().x2, text_font.line_height() + 1).center_vertical(ttgo_default_screen.bounds()));
    text.font(text_font);
    text.measure_cache(text_measure_cache);
    text.draw_cache(text_draw_cache);
    text.color(uix_color_t::white);
    text.text_justify(uix_justify::bottom_middle);
    ttgo_default_screen.register_control(text);

    const int16_t dot_size = text.bounds().y1 / 2;

    // dot 0 control
    dot0.bounds(srect16(0, 0, dot_size * 1.5 - 1, dot_size - 1));
    dot0.on_paint_callback(dot_on_paint, (void*)0);
    dot0.visible(false);
    ttgo_default_screen.register_control(dot0);

    // dot 35 control
    dot35.bounds(srect16(0, 0, dot_size * 1.5 - 1, dot_size - 1).offset(0, ttgo_default_screen.dimensions().height - dot_size));
    dot35.on_paint_callback(dot_on_paint, (void*)35);
    dot35.visible(false);
    ttgo_default_screen.register_control(dot35);

    // battery control
    batt.bounds(srect16(0, 0, dot_size * 2 - 1, dot_size - 1).offset(ttgo_default_screen.dimensions().width - (dot_size * 2), 0));
    batt.color(uix_color_t::white);
    batt.inner_color(uix_color_t::green);
    batt.draw_cache(draw_cache);
    batt.visible(false);
    ttgo_default_screen.register_control(batt);

    // kick the text off
    strcpy(text_data, "start clicking!");
    start_text();

    // start the app loop
    TaskHandle_t loop_handle;
    xTaskCreate(loop_task, "loop_task", 4096, nullptr, uxTaskPriorityGet(xTaskGetCurrentTaskHandle()), &loop_handle);
}
```
[Go to full PlatformIO demo with all files](https://github.com/codewitch-honey-crisis/ttgo_multibutton)

For PlatformIO or the Espressif repository:

codewitch-honey-crisis/htcw_ttgo
