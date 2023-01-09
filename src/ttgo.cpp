#include <ttgo.hpp>
lcd_t lcd;
button_a_t button_a_raw;
button_b_t button_b_raw;
arduino::multi_button button_a(button_a_raw);
arduino::multi_button button_b(button_b_raw);
dimmer_t dimmer;
void ttgo_initialize() {
    lcd.initialize();
    button_a.initialize();
    button_b.initialize();
    dimmer.initialize();
}
void ttgo_update() {
    button_a.update();
    button_b.update();
    dimmer.update();
}
