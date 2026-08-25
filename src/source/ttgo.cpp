#include "ttgo.hpp"

#include <driver/gpio.h>
#include <esp_check.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <memory.h>

#include "esp_lcd_panel_io.h"
#include "multibutton.h"
#ifndef TTGO_BUTTON_EVENTS
#define TTGO_BUTTON_EVENTS MULTIBUTTON_EVENT_SIZE_DEFAULT
#endif
ttgo_screen_t ttgo_default_screen;
static multibutton_t mb_0_data;
static multibutton_handle_t mb_0_handle;
static multibutton_event_t mb_0_events[TTGO_BUTTON_EVENTS];
static multibutton_t mb_35_data;
static multibutton_handle_t mb_35_handle;
static multibutton_event_t mb_35_events[TTGO_BUTTON_EVENTS];
static bool mb_0_old_pressed = false;
static bool mb_35_old_pressed = false;
#ifdef LCD_BUS

uix::display ttgo_display;

static void uix_flush(const gfx::rect16& bounds, const void* bmp, void* state) {
    panel_lcd_flush(bounds.x1, bounds.y1, bounds.x2, bounds.y2, (void*)bmp);
#if LCD_SYNC_TRANSFER == 1
    ttgo_display.flush_complete();
#endif
}

#endif
__attribute__((weak)) void ttgo_on_pressed_changed(uint8_t gpio, bool pressed) {
}
__attribute__((weak)) void ttgo_on_clicks(uint8_t gpio, unsigned clicks) {
}
__attribute__((weak)) void ttgo_on_long_click(uint8_t gpio) {
}
__attribute__((weak)) void ttgo_on_lcd_enabled_changed(bool enabled) {
}

static void mb_on_pressed_changed(bool pressed, void* state) {
    ttgo_on_pressed_changed((int)state, pressed);
}
static void mb_on_clicks(unsigned clicks, void* state) {
    ttgo_on_clicks((int)state, clicks);
}
static void mb_on_long_click(void* state) {
    ttgo_on_long_click((int)state);
}

void ttgo_init(ttgo_options_t options) {
#ifdef POWER
    panel_power_init();
#endif
#ifdef LCD_BUS
    panel_lcd_init();
#endif
#ifdef BUTTON
    panel_button_init();
#endif
#ifdef LCD_BUS
    ttgo_display.buffer_size(LCD_TRANSFER_SIZE);
    ttgo_display.buffer1((uint8_t*)panel_lcd_transfer_buffer());
#if LCD_SYNC_TRANSFER == 0
    ttgo_display.buffer2((uint8_t*)panel_lcd_transfer_buffer2());
#endif
    ttgo_display.on_flush_callback(uix_flush);
#endif

    multibutton_config_t cfg;
    cfg.double_click = 0;
    cfg.long_click = 0;
    cfg.on_pressed_changed_callback = mb_on_pressed_changed;
    cfg.on_pressed_changed_callback_state = (void*)TTGO_BUTTON_0;
    if ((options & TTGO_BUTTON_0_CLICKS) == TTGO_BUTTON_0_CLICKS) {
        cfg.double_click = 200;
        cfg.on_clicks_callback = mb_on_clicks;
        cfg.on_clicks_callback_state = (void*)TTGO_BUTTON_0;
    } else {
        cfg.on_clicks_callback = nullptr;
    }
    if ((options & TTGO_BUTTON_0_LONG) == TTGO_BUTTON_0_LONG) {
        cfg.long_click = 500;
        cfg.on_long_click_callback = mb_on_long_click;
        cfg.on_long_click_callback_state = (void*)TTGO_BUTTON_0;
    } else {
        cfg.on_long_click_callback = nullptr;
    }
    cfg.events_size = TTGO_BUTTON_EVENTS;
    mb_0_handle = multibutton_init_za(&cfg, mb_0_events, &mb_0_data);
    cfg.double_click = 0;
    cfg.long_click = 0;
    cfg.on_pressed_changed_callback_state = (void*)TTGO_BUTTON_35;
    if ((options & TTGO_BUTTON_35_CLICKS) == TTGO_BUTTON_35_CLICKS) {
        cfg.double_click = 200;
        cfg.on_clicks_callback = mb_on_clicks;
        cfg.on_clicks_callback_state = (void*)TTGO_BUTTON_35;
    } else {
        cfg.on_clicks_callback = nullptr;
    }
    if ((options & TTGO_BUTTON_35_LONG) == TTGO_BUTTON_35_LONG) {
        cfg.long_click = 500;
        cfg.on_long_click_callback = mb_on_long_click;
        cfg.on_long_click_callback_state = (void*)TTGO_BUTTON_35;
    } else {
        cfg.on_long_click_callback = nullptr;
    }
    mb_35_handle = multibutton_init_za(&cfg, mb_35_events, &mb_35_data);
    ttgo_default_screen.dimensions(TTGO_LCD_DIM);
    ttgo_display.active_screen(ttgo_default_screen);
}

bool ttgo_pressed(uint8_t gpio) {
    if (gpio == 0) {
        return multibutton_pressed(mb_0_handle);
    } else if (gpio == 35) {
        return multibutton_pressed(mb_35_handle);
    }
    return false;
}
// Convert a battery voltage (mV) to an estimated percentage (0-100).
uint8_t ttgo_battery_level(void) {
    return panel_power_battery_level();  // unreachable
}
void ttgo_power_off(void) {
    panel_power_off();
}
static uint8_t lcd_backlight_percent = 100;
static uint8_t lcd_fade_level = 0;
static bool lcd_enabled = false;
bool ttgo_lcd_enabled(void) {
    return lcd_enabled;
}
void ttgo_lcd_enable(bool value) {
    if (value != lcd_enabled) {
        uint8_t cmd = 0x10 | value;
        esp_lcd_panel_io_handle_t h = (esp_lcd_panel_io_handle_t)panel_lcd_io_handle();
        if (h != nullptr) {
            esp_lcd_panel_io_tx_param(h, cmd, nullptr, 0);
        }
        lcd_enabled = value;
        ttgo_on_lcd_enabled_changed(value);
    }
    lcd_fade_level = 0;
    panel_lcd_backlight(value * (lcd_backlight_percent * 255 / 100));
}

void ttgo_backlight(uint8_t percent) {
    if (percent > 100) percent = 100;
    lcd_backlight_percent = percent;
    panel_lcd_backlight(percent * 255 / 100);
}
static TickType_t lcd_fade_ts = 0;
void ttgo_lcd_fade_to_sleep(void) {
    lcd_fade_level = lcd_backlight_percent * 255 / 100;
}

void ttgo_update(void) {
    bool pressed = panel_button_read(TTGO_BUTTON_0);
    if (pressed != mb_0_old_pressed) {
        multibutton_event(mb_0_handle, pdTICKS_TO_MS(xTaskGetTickCount()), pressed);
        mb_0_old_pressed = pressed;
    }
    pressed = panel_button_read(TTGO_BUTTON_35);
    if (pressed != mb_35_old_pressed) {
        multibutton_event(mb_35_handle, pdTICKS_TO_MS(xTaskGetTickCount()), pressed);
        mb_35_old_pressed = pressed;
    }
    multibutton_update(mb_0_handle, pdTICKS_TO_MS(xTaskGetTickCount()));
    multibutton_update(mb_35_handle, pdTICKS_TO_MS(xTaskGetTickCount()));
    if (lcd_fade_level > 0) {
        if (xTaskGetTickCount() >= lcd_fade_ts + pdMS_TO_TICKS(5)) {
            lcd_fade_ts = xTaskGetTickCount();
            --lcd_fade_level;
            panel_lcd_backlight(lcd_fade_level);
            if (lcd_fade_level == 0) {
                ttgo_lcd_enable(false);
                lcd_fade_ts = 0;
            }
        }
    }
#ifdef LCD_BUS
    ttgo_display.update();
#endif
}

#ifdef LCD_BUS
void panel_lcd_flush_complete() {
    ttgo_display.flush_complete();
}
#endif
