Preliminary version of esphome component for LiteOn LTR-303 and LTR-329 light sensors.

Example config:
```
esphome:
  name: ltr303-test

esp32:
  board: esp32dev
  framework:
    type: arduino

# esp8266:
#   board: nodemcuv2

logger:
  level: DEBUG

external_components:
  - source: github://latonita/esphome-ltr303
    components: [ ltr_als_ps ]
#   - source: github://pr#6076
#     components: [ltr_als_ps]

i2c:
  sda: GPIO25
  scl: GPIO32
  scan: true

sensor:
  - platform: ltr_als_ps
    address: 0x29
    auto_mode: true
# gain and time ignored in auto mode
    gain: 1x
    integration_time: 100ms
    glass_attenuation_factor: 1.0
    ambient_light: Ambient light
# Following sensors are not really of a lot of use, to be honest :)
    full_spectrum_counts: Full spectrum counts
    infrared_counts: Infrared counts
    actual_gain: Actual gain
    actual_integration_time: Actual integration time
```
