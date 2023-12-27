#pragma once

#include <tuple>
#include <vector>
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"
#include "esphome/core/optional.h"

namespace esphome {
namespace ltr303 {

enum LTR303CTRL {
    LTR303_CTRL_ALS_ACTIVE = 1,
    LTR303_CTRL_SW_RESET = 2,
};

// https://www.mouser.com/datasheet/2/239/Lite-On_LTR-303ALS-01_DS_ver%201.1-1175269.pdf

static const uint8_t LTR303_ALS_CTRL = 0x80;         // ALS operation mode control SW reset
static const uint8_t LTR303_MEAS_RATE = 0x85;        // ALS measurement rate in active mode
static const uint8_t LTR303_GAIN_MASK = 0b00011100;  // Gain is part of ALS control register
static const uint8_t LTR303_PART_ID = 0x86;          // Part Number ID and Revision ID
static const uint8_t LTR303_MANU_ID = 0x87;          // Manufacturer ID
static const uint8_t LTR303_CH1_0 = 0x88;            // ALS measurement CH1 data, lower byte - infrared only
static const uint8_t LTR303_CH1_1 = 0x89;            // ALS measurement CH1 data, upper byte - infrared only
static const uint8_t LTR303_CH0_0 = 0x8A;            // ALS measurement CH0 data, lower byte - visible + infrared
static const uint8_t LTR303_CH0_1 = 0x8B;            // ALS measurement CH0 data, upper byte - visible + infrared
static const uint8_t LTR303_ALS_STATUS = 0x8c;       // ALS new data status

// For interrupt mode (303 only)
// static const uint8_t LTR303_INTERRUPT = 0x8F;        // ALS interrupt upper threshold, lower byte
// static const uint8_t LTR303_ALS_THRES_UP_0 = 0x97;   // ALS interrupt upper threshold, lower byte
// static const uint8_t LTR303_ALS_THRES_UP_1 = 0x98;   // ALS interrupt upper threshold, upper byte
// static const uint8_t LTR303_ALS_THRES_LOW_0 = 0x99;  // ALS interrupt lower threshold, lower byte
// static const uint8_t LTR303_ALS_THRES_LOW_1 = 0x9A;  // ALS interrupt lower threshold, upper byte

// Sensor gain levels
enum LTR303Gain {
    LTR303_GAIN_1 = 0,  // default
    LTR303_GAIN_2 = 1,
    LTR303_GAIN_4 = 2,
    LTR303_GAIN_8 = 3,
    LTR303_GAIN_48 = 6,
    LTR303_GAIN_96 = 7,
};

enum LTR303IntegrationTime {
    LTR303_INTEGRATION_TIME_100MS = 0,  // default
    LTR303_INTEGRATION_TIME_50MS = 1,
    LTR303_INTEGRATION_TIME_200MS = 2,
    LTR303_INTEGRATION_TIME_400MS = 3,
    LTR303_INTEGRATION_TIME_150MS = 4,
    LTR303_INTEGRATION_TIME_250MS = 5,
    LTR303_INTEGRATION_TIME_300MS = 6,
    LTR303_INTEGRATION_TIME_350MS = 7
};

enum LTR303MeasurementRepeatRate {
    LTR303_MEASUREMENT_REPEAT_RATE_50MS = 0,
    LTR303_MEASUREMENT_REPEAT_RATE_100MS = 1,
    LTR303_MEASUREMENT_REPEAT_RATE_200MS = 2,
    LTR303_MEASUREMENT_REPEAT_RATE_500MS = 3,  // default
    LTR303_MEASUREMENT_REPEAT_RATE_1000MS = 4,
    LTR303_MEASUREMENT_REPEAT_RATE_2000MS = 5
};

class LTR303Component : public PollingComponent, public i2c::I2CDevice {
   public:
    float get_setup_priority() const override { return setup_priority::DATA; }
    void setup() override;
    void dump_config() override;
    void update() override;

    void set_gain_value(LTR303Gain gain) { this->gain_ = gain; }
    void set_integration_time_value(LTR303IntegrationTime time) { this->integration_time_ = time; }
    void set_repeat_rate_value(LTR303MeasurementRepeatRate rate) { this->repeat_rate_ = rate; }

    void set_infrared_sensor(sensor::Sensor *sensor) { this->infrared_sensor_ = sensor; }
    void set_full_spectrum_sensor(sensor::Sensor *sensor) { this->full_spectrum_sensor_ = sensor; }
    void set_actual_gain_sensor(sensor::Sensor *sensor) { this->actual_gain_sensor_ = sensor; }
    void set_calculated_lux_sensor(sensor::Sensor *sensor) { this->calculated_lux_sensor_ = sensor; }

   protected:
    void reset_();

    bool read_sensor_data_();

    float calculate_lux_(uint16_t channel0, uint16_t channel1, uint8_t gain, uint8_t time);

    bool reading_{false};

    uint16_t channel0{0};
    uint16_t channel1{0};
    float lux{0};

    LTR303Gain gain_{LTR303_GAIN_1};
    LTR303IntegrationTime integration_time_{LTR303_INTEGRATION_TIME_100MS};
    LTR303MeasurementRepeatRate repeat_rate_{LTR303_MEASUREMENT_REPEAT_RATE_500MS};

    sensor::Sensor *infrared_sensor_{nullptr};        // direct reading CH1, infrared only
    sensor::Sensor *full_spectrum_sensor_{nullptr};   // direct reading CH0, infrared + visible light
    sensor::Sensor *actual_gain_sensor_{nullptr};     // actual gain of reading
    sensor::Sensor *calculated_lux_sensor_{nullptr};  // calculated lux

    enum ErrorCode { NONE = 0, COMMUNICATION_FAILED } error_code_{NONE};
};

}  // namespace ltr303
}  // namespace esphome
