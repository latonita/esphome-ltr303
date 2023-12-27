#include "ltr303.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include <bitset>

namespace esphome {
namespace ltr303 {

static const char *const TAG = "ltr303";

static const int ALS_GAIN[8] = {1, 2, 4, 8, 1, 1, 48, 96};  // no gain option between x8 and x48
static const int ALS_INT_TIME[8] = {100, 50, 200, 400, 150, 250, 300, 350};
static const int ALS_MEAS_RATE[8] = {50, 100, 200, 500, 1000, 2000, 2000, 2000};

/*
Formula for device in open air WITHOUT aperture or window above the device:
RATIO = CH1/(CH0+CH1)
IF (RATIO < 0.45)
  ALS_LUX = (1.7743 * CH0 + 1.1059 * CH1) / ALS_GAIN / ALS_INT
ELSEIF (RATIO < 0.64 && RATIO >= 0.45)
  ALS_LUX = (4.2785 * CH0 – 1.9548 * CH1) / ALS_GAIN / ALS_INT
ELSEIF (RATIO < 0.85 && RATIO >= 0.64)
  ALS_LUX = (0.5926 * CH0 + 0.1185 * CH1) / ALS_GAIN / ALS_INT
ELSE
  ALS_LUX = 0
END
*/

float LTR303Component::calculate_lux_(uint16_t channel0, uint16_t channel1, uint8_t gain, uint8_t time) {
    if ((channel0 == 0xFFFF) || (channel1 == 0xFFFF)) {
        ESP_LOGW(TAG, "Sensors got saturated");
        return 0.0;
    }

    if ((channel0 == 0x0000) && (channel1 == 0x0000)) {
        ESP_LOGW(TAG, "Sensors blacked out");
        return 0.0;
    }

    double ch0 = channel0;
    double ch1 = channel1;
    double ratio = ch1 / (ch0 + ch1);
    double als_gain = ALS_GAIN[gain];
    double als_int = ALS_INT_TIME[time] / 100;
    double lux = 0;

    if (ratio < 0.45) {
        lux = (1.7743 * ch0 + 1.1059 * ch1) / als_gain / als_int;
    } else if (ratio < 0.64 && ratio >= 0.45) {
        lux = (4.2785 * ch0 - 1.9548 * ch1) / als_gain / als_int;
    } else if (ratio < 0.85 && ratio >= 0.64) {
        lux = (0.5926 * ch0 + 0.1185 * ch1) / als_gain / als_int;
    } else {
        lux = 0;
    }
    return lux;
}

/*

Formula for device in open air WITH aperture or window above the device:
RATIO = CH1/(CH0+CH1)
IF (RATIO < 0.45)
  ALS_LUX = (1.7743 * CH0 + 1.1059 * CH1) / ALS_GAIN / ALS_INT / PFactor
ELSEIF (RATIO < 0.64 && RATIO >= 0.45)
  ALS_LUX = (4.2785 * CH0 – 1.9548 * CH1) / ALS_GAIN / ALS_INT / PFactor
ELSEIF (RATIO < 0.85 && RATIO >= 0.64)
  ALS_LUX = (0.5926 * CH0 + 0.1185 * CH1) / ALS_GAIN / ALS_INT / PFactor
ELSE
  ALS_LUX = 0
END

*/

bool LTR303Component::read_sensor_data_() {
    ESP_LOGD(TAG, "Reading sendsor data");

    uint8_t status = 0;

    const uint32_t now = millis();
    while (true) {
        status = this->reg(LTR303_ALS_STATUS).get();
        ESP_LOGD(TAG, "LTR303_ALS_STATUS = 0x%0x", status);
        // check for data status bit
        if (status & 0b100) {
            ESP_LOGD(TAG, "Data ready status bit set, good");
            break;
        }
        if (millis() - now > 100) {
            ESP_LOGW(TAG, "No data returned from sensor");
            return false;
        }
        ESP_LOGW(TAG, "Waiting for data");
        delay(5);
    }

    // got data, check validity
    if (!(status & 0b10000000)) {
        ESP_LOGW(TAG, "Data available but not valid");
        return false;
    }

    // finally
    uint16_t ch1_0 = this->reg(LTR303_CH1_0).get();
    uint16_t ch1_1 = this->reg(LTR303_CH1_1).get();
    uint16_t ch0_0 = this->reg(LTR303_CH0_0).get();
    uint16_t ch0_1 = this->reg(LTR303_CH0_1).get();

    this->channel1 = (ch1_1 << 8) | ch1_0;
    this->channel0 = (ch0_1 << 8) | ch0_0;

    ESP_LOGD(TAG, "Channel data CH1 = %d, CH0 = %d", this->channel1, this->channel0);

    // check actual gain in status
    uint8_t actual_gain = (status & 0b01110000) >> 4;
    if (actual_gain != this->gain_) {
        ESP_LOGW(TAG, "Actual gain (%d) differs from requested (%d)", ALS_GAIN[actual_gain], ALS_GAIN[this->gain_]);
    }

    if (this->infrared_sensor_ != nullptr) {
        this->infrared_sensor_->publish_state(this->channel1);
    }

    if (this->full_spectrum_sensor_ != nullptr) {
        this->full_spectrum_sensor_->publish_state(this->channel0);
    }

    if (this->actual_gain_sensor_ != nullptr) {
        this->actual_gain_sensor_->publish_state(actual_gain);
    }

    if (this->calculated_lux_sensor_ != nullptr) {
        float lux = this->calculate_lux_(this->channel0, this->channel1, actual_gain, this->integration_time_);
        ESP_LOGD(TAG, "Calculated lux: %f", lux);
        this->calculated_lux_sensor_->publish_state(lux);
    }

    return true;
}

void LTR303Component::reset_() {
    ESP_LOGD(TAG, "Resetting...");
    this->reg(LTR303_ALS_CTRL) = LTR303CTRL::LTR303_CTRL_SW_RESET;
    delay(5);

    uint8_t ctr = 0;
    uint8_t tries = 10;

    do {
        delay(5);
        ctr = this->reg(LTR303_ALS_CTRL).get();
    } while (ctr & LTR303CTRL::LTR303_CTRL_SW_RESET && tries);  // while reset bit is on keep waitin
}

void LTR303Component::setup() {
    ESP_LOGCONFIG(TAG, "Setting up ltr303...");
    delay(120);  // replace with nonblocking

    this->reset_();

    uint8_t cmd = LTR303CTRL::LTR303_CTRL_ALS_ACTIVE | (this->gain_ << 2);
    ESP_LOGD(TAG, "Setting active mode and gain 0x%0x", cmd);

    this->reg(LTR303_ALS_CTRL) = LTR303CTRL::LTR303_CTRL_ALS_ACTIVE | (this->gain_ << 2);
    delay(12);

    // check/wait active
    // todo

    // set int time and rate
    cmd = (this->integration_time_ << 3) | (this->repeat_rate_);
    ESP_LOGD(TAG, "Setting integration time and measurement repeat rate 0x%0x", cmd);
    this->reg(LTR303_MEAS_RATE) = (this->integration_time_ << 3) | (this->repeat_rate_);
    delay(12);

    auto err = this->write(nullptr, 0);
    if (err != i2c::ERROR_OK) {
        ESP_LOGD(TAG, "i2c connection failed");

        this->error_code_ = COMMUNICATION_FAILED;
        this->mark_failed();
        return;
    }
}

void LTR303Component::dump_config() {
    LOG_I2C_DEVICE(this);
    ESP_LOGCONFIG(TAG, "  Gain: x%d", ALS_GAIN[this->gain_]);
    ESP_LOGCONFIG(TAG, "  Integration time: %d ms", ALS_INT_TIME[this->integration_time_]);
    ESP_LOGCONFIG(TAG, "  Measurement repeat rate: %d ms", ALS_MEAS_RATE[this->repeat_rate_]);
    LOG_UPDATE_INTERVAL(this);

    if (this->error_code_ == COMMUNICATION_FAILED) {
        ESP_LOGE(TAG, "Communication with I2C LTR303/329 failed!");
    }
}

void LTR303Component::update() {
    ESP_LOGD(TAG, "Updating");

    if (!this->reading_) {
        this->reading_ = true;
        this->read_sensor_data_();
        this->reading_ = false;
    }
}

}  // namespace ltr303
}  // namespace esphome
