# ttgo

This library allows you to use the TTGO Display T1 devkit

```
[env:ttgo-demo]
platform = espressif32
board = ttgo-t1
framework = arduino
monitor_speed = 115200
monitor_filters = esp32_exception_decoder
upload_speed = 921600
lib_ldf_mode = deep
lib_deps = codewitch-honey-crisis/htcw_ttgo
build_unflags = -std=gnu++11
build_flags = -std=gnu++17
```