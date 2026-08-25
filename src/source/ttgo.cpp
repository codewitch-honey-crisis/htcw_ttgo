#include <memory.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <driver/ledc.h>
#include <esp_check.h>
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include "ttgo.hpp"
#include "ttgo_power.h"
#include "multibutton.h"
#ifndef TTGO_BUTTON_EVENTS
#define TTGO_BUTTON_EVENTS MULTIBUTTON_EVENT_SIZE_DEFAULT
#endif
#define LCD_TRANSFER_SIZE ((120*135*16+7)/8)
#define LCD_PIN_NUM_MOSI 19
#define LCD_PIN_NUM_CLK 18
#define LCD_PIN_NUM_CS 5
#define LCD_PIN_NUM_DC 16
#define LCD_PIN_NUM_RST 23
#define LCD_PIN_NUM_BCKL 4
#define LCD_BCKL_PWM_CHANNEL 0

#define LCD_GAP_X 40
#define LCD_GAP_Y 52
#define LCD_MIRROR_X 0
#define LCD_MIRROR_Y 1
#define LCD_INVERT_COLOR 1
#define LCD_SWAP_XY 1

ttgo_screen_t ttgo_default_screen;
static esp_lcd_panel_handle_t lcd_handle = NULL;
static esp_lcd_panel_io_handle_t lcd_io_handle = NULL;
static uint8_t lcd_xfer_buffer[LCD_TRANSFER_SIZE];
static uint8_t lcd_xfer_buffer2[LCD_TRANSFER_SIZE];
static multibutton_t mb_0_data;
static multibutton_handle_t mb_0_handle;
static multibutton_event_t mb_0_events[TTGO_BUTTON_EVENTS];
static multibutton_t mb_35_data;
static multibutton_handle_t mb_35_handle;
static multibutton_event_t mb_35_events[TTGO_BUTTON_EVENTS];
static bool mb_0_old_pressed = false;
static bool mb_35_old_pressed = false;

uix::display ttgo_display;

static void uix_flush(const gfx::rect16& bounds, const void* bmp, void* state) {
    esp_lcd_panel_draw_bitmap(lcd_handle,bounds.x1, bounds.y1, bounds.x2+1, bounds.y2+1, bmp);
}

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
static IRAM_ATTR bool on_flush_complete(esp_lcd_panel_io_handle_t lcd_io, esp_lcd_panel_io_event_data_t* edata, void* user_ctx) {
    ttgo_display.flush_complete();
    return true;
}
void ttgo_init(ttgo_options_t options) {
    ttgo_power_init();
    gpio_config_t gpio_cfg = {};
    gpio_cfg.mode = GPIO_MODE_INPUT;
    gpio_cfg.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_cfg.pin_bit_mask = (1ULL << TTGO_BUTTON_0) | (1ULL << TTGO_BUTTON_35);
    ESP_ERROR_CHECK(gpio_config(&gpio_cfg));
    ESP_ERROR_CHECK(gpio_set_pull_mode((gpio_num_t)TTGO_BUTTON_0,GPIO_PULLUP_ONLY));
    ledc_timer_config_t ledc_timer;
    memset(&ledc_timer,0,sizeof(ledc_timer)); 
    ledc_timer.speed_mode       = LEDC_LOW_SPEED_MODE;
    ledc_timer.duty_resolution  = LEDC_TIMER_8_BIT;
    ledc_timer.timer_num        = LEDC_TIMER_0;
    ledc_timer.freq_hz          = 5000;
    ledc_timer.clk_cfg          = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
    ledc_channel_config_t ledc_cfg;
    memset(&ledc_cfg,0,sizeof(ledc_cfg));
    ledc_cfg.channel = (ledc_channel_t)LCD_BCKL_PWM_CHANNEL;
    ledc_cfg.duty = 0;
    ledc_cfg.hpoint = 0;
    ledc_cfg.speed_mode = LEDC_LOW_SPEED_MODE;
    ledc_cfg.gpio_num = LCD_PIN_NUM_BCKL;
    ledc_cfg.timer_sel = LEDC_TIMER_0;
    ledc_cfg.flags.output_invert= 0;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_cfg));

    spi_bus_config_t spi_cfg;
    memset(&spi_cfg,0,sizeof(spi_cfg));
    spi_cfg.data0_io_num = LCD_PIN_NUM_MOSI;
    spi_cfg.data1_io_num = -1;
    spi_cfg.data2_io_num = -1;
    spi_cfg.data3_io_num = -1;
    spi_cfg.data4_io_num = -1;
    spi_cfg.data5_io_num = -1;
    spi_cfg.data6_io_num = -1;
    spi_cfg.data7_io_num = -1;
    spi_cfg.sclk_io_num = LCD_PIN_NUM_CLK;
    spi_cfg.max_transfer_sz = LCD_TRANSFER_SIZE+8;
    spi_bus_initialize((spi_host_device_t)SPI3_HOST,&spi_cfg,SPI_DMA_CH_AUTO);
    
    esp_lcd_panel_io_spi_config_t lcd_spi_cfg;
    memset(&lcd_spi_cfg,0,sizeof(lcd_spi_cfg));
    lcd_spi_cfg.cs_gpio_num = (gpio_num_t)LCD_PIN_NUM_CS;
    lcd_spi_cfg.dc_gpio_num = (gpio_num_t)LCD_PIN_NUM_DC;
    lcd_spi_cfg.lcd_cmd_bits = 8;
    lcd_spi_cfg.lcd_param_bits = 8;        
    lcd_spi_cfg.pclk_hz = 40 * 1000 * 1000;
    lcd_spi_cfg.trans_queue_depth = 10;
    lcd_spi_cfg.on_color_trans_done = on_flush_complete;
    lcd_spi_cfg.spi_mode = 0;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI3_HOST, &lcd_spi_cfg, &lcd_io_handle));
    esp_lcd_panel_dev_config_t panel_config;
    memset(&panel_config,0,sizeof(panel_config));
    panel_config.reset_gpio_num = (gpio_num_t)LCD_PIN_NUM_RST;
    panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
    panel_config.data_endian = LCD_RGB_DATA_ENDIAN_BIG;
    panel_config.bits_per_pixel = 16;
    panel_config.vendor_config = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(lcd_io_handle, &panel_config, &lcd_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(lcd_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(lcd_handle));
    int gap_x = 0, gap_y = 0;
#ifdef LCD_GAP_X
    gap_x = LCD_GAP_X;
#endif
#ifdef LCD_GAP_Y
    gap_y = LCD_GAP_Y;
#endif
    esp_lcd_panel_set_gap(lcd_handle,gap_x,gap_y);
    
#ifdef LCD_SWAP_XY
#if LCD_SWAP_XY
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(lcd_handle,true));
#endif
#endif
    bool mirror_x = false, mirror_y = false;
#ifdef LCD_MIRROR_X
#if LCD_MIRROR_X
    mirror_x = LCD_MIRROR_X;
#endif
#endif
#ifdef LCD_MIRROR_Y
#if LCD_MIRROR_Y
    mirror_y = LCD_MIRROR_Y;
#endif
#endif
    esp_lcd_panel_mirror(lcd_handle,mirror_x,mirror_y);
    
#ifdef LCD_INVERT_COLOR
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(lcd_handle,LCD_INVERT_COLOR));
#endif
    esp_lcd_panel_disp_on_off(lcd_handle, true);
    ledc_set_duty(LEDC_LOW_SPEED_MODE,(ledc_channel_t)LCD_BCKL_PWM_CHANNEL,255);
    ledc_update_duty(LEDC_LOW_SPEED_MODE,(ledc_channel_t)LCD_BCKL_PWM_CHANNEL);
    ttgo_display.buffer_size(LCD_TRANSFER_SIZE);
    ttgo_display.buffer1(lcd_xfer_buffer);
    ttgo_display.buffer2(lcd_xfer_buffer2);
    ttgo_display.on_flush_callback(uix_flush);

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
    return ttgo_power_level();  // unreachable
}
void ttgo_power_off(void) {
    ttgo_power_enable(false);
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
        esp_lcd_panel_io_handle_t h = lcd_io_handle;
        if (h != nullptr) {
            esp_lcd_panel_io_tx_param(h, cmd, nullptr, 0);
        }
        lcd_enabled = value;
        ttgo_on_lcd_enabled_changed(value);
    }
    lcd_fade_level = 0;
    ledc_set_duty(LEDC_LOW_SPEED_MODE,(ledc_channel_t)LCD_BCKL_PWM_CHANNEL,value * (lcd_backlight_percent * 255 / 100));
}

void ttgo_backlight(uint8_t percent) {
    if (percent > 100) percent = 100;
    lcd_backlight_percent = percent;
    ledc_set_duty(LEDC_LOW_SPEED_MODE,(ledc_channel_t)LCD_BCKL_PWM_CHANNEL,percent * 255 / 100);
    ledc_update_duty(LEDC_LOW_SPEED_MODE,(ledc_channel_t)LCD_BCKL_PWM_CHANNEL);
}
static TickType_t lcd_fade_ts = 0;
void ttgo_lcd_fade_to_sleep(void) {
    lcd_fade_level = lcd_backlight_percent * 255 / 100;
}

void ttgo_update(void) {
    bool pressed = !gpio_get_level((gpio_num_t)TTGO_BUTTON_0);
    if (pressed != mb_0_old_pressed) {
        multibutton_event(mb_0_handle, pdTICKS_TO_MS(xTaskGetTickCount()), pressed);
        mb_0_old_pressed = pressed;
    }
    pressed = !gpio_get_level((gpio_num_t)TTGO_BUTTON_35);
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
            ledc_set_duty(LEDC_LOW_SPEED_MODE,(ledc_channel_t)LCD_BCKL_PWM_CHANNEL, lcd_fade_level);
            ledc_update_duty(LEDC_LOW_SPEED_MODE,(ledc_channel_t)LCD_BCKL_PWM_CHANNEL);
            if (lcd_fade_level == 0) {
                ttgo_lcd_enable(false);
                lcd_fade_ts = 0;
            }
        }
    }
    ttgo_display.update();
}


