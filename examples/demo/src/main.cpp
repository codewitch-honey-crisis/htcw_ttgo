#include <ttgo.hpp>
// downloaded from fontsquirrel.com and header generated with 
// https://honeythecodewitch.com/gfx/generator
#include <fonts/Telegrama.hpp>
static void button_a_on_click(int clicks,void* state) {
    // reset the dimmer
    dimmer.wake();
}
static void button_b_on_click(int clicks,void* state) {
    // reset the dimmer
    dimmer.wake();
}
void setup() {
    Serial.begin(115200);
    ttgo_initialize();
    lcd.rotation(1);
    // set the button callbacks
    button_a.on_click(button_a_on_click);
    button_b.on_click(button_b_on_click);
    
    draw::filled_rectangle(lcd,lcd.bounds(),color_t::purple);
    open_text_info oti;
    oti.font = &Telegrama;
    oti.text = "TTGO T1";
    oti.scale = oti.font->scale(35);
    oti.transparent_background = false;
    srect16 txtr = oti.font->measure_text(ssize16::max(),spoint16::zero(),oti.text,oti.scale).bounds();
    txtr.center_inplace((srect16)lcd.bounds());
    draw::text(lcd,txtr,oti,color_t::white,color_t::purple);
}

void loop() {
    ttgo_update();
}