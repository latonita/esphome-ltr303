Preliminary version of esphome component for LiteOn LTR-553 light sensor [ALS] and proximity sensor [PS].

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
  - source: github://latonita/esphome-ltr303@alsps
    components: [ ltr303 ]
#   - source: github://pr#6076
#     components: [ltr303]

i2c:
  sda: GPIO25
  scl: GPIO32
  scan: true

sensor:
  - platform: ltr303
    address: 0x23
    auto_mode: false

# for ltr-553 
    enable_proximity: true
    proximity_cooldown: 5 s
    proximity_high_threshold: 590
    proximity_low_threshold: 5
    on_high_threshold:
      then:
        - logger.log: "Proximity high!                                             !!!!!!!!!!!!!!!!"
    on_low_threshold:
      then:
        - logger.log: "Proximity low high                                          ----------------"

    proximity_counts:
      name: "Proximity counts"    

# gain and time ignored in auto mode
    gain: 1x
    integration_time: 100ms

    glass_attenuation_factor: 1.0
    ambient_light:
      name: "Ambient light"
# Following sensors are not really of a lot of use, to be honest :)
    full_spectrum_counts:
      name: "Full spectrum counts"
    infrared_counts:
      name: "Infrared counts"
    actual_gain:
      name: "Actual gain"
    actual_integration_time:
      name: "Actual integration time"
```
