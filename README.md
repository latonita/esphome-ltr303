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
    components: [ ltr303 ]

i2c:
  sda: GPIO25
  scl: GPIO32
  scan: true

sensor:
  - platform: ltr303
    address: 0x29
    gain: X8
    integration_time: 100ms
    repeat: 500ms
    infrared:
      name: "Infrared"
    full_spectrum:
      name: "Visible + Infrared"
    actual_gain:
      name: "Actual gain"
    calculated_lux:
      name: "Calculated lux"
```
