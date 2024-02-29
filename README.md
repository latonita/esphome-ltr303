Preliminary version of esphome component for Lite-On TR-303, LTR-329, LTR-553, LTR-556, LTR-559, 
LTR-659 Series of Lite-On Light (ALS) and Proximity(PS) sensors.

PR for official esphome repo still under review https://github.com/esphome/esphome/pull/6076.

Full documentation is here: https://deploy-preview-3528--esphome.netlify.app/components/sensor/ltr_als_ps


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
    refresh: 1h
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
sensor:
  - platform: ltr_als_ps
    address: 0x29
    auto_mode: true
    type: ALS   # ALS, PS, ALS_PS

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

# proximity section
#    ps_cooldown: 3 s
#    ps_high_threshold: 590
#    ps_low_threshold: 10
#    on_ps_high_threshold:
#      then:
#        - logger.log: "Proximity high threshold"
#    on_ps_low_threshold:
#      then:
#        - logger.log: "Proximity low threshold"
#    ps_counts: Proximity counts
```
