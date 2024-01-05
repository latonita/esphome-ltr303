#include "ltr303.h"
#include "esphome/core/application.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ltr303 {

static const char *const TAG = "ltr303";

static const int ALS_GAIN[8] = {1, 2, 4, 8, 1, 1, 48, 96};  // no gain option between x8 and x48
static const int ALS_INT_TIME[8] = {100, 50, 200, 400, 150, 250, 300, 350};
static const int ALS_MEAS_RATE[8] = {50, 100, 200, 500, 1000, 2000, 2000, 2000};

/*

LTR-329 Appendix A

The ALS lux reading is a function of both count data in ALS_DATA_CH0 and
ALS_DATA_CH1 registers, ALS Gain in ALS_CONTR register (0x80) and ALS Integration
Time in ALS_MEAS_RATE Register (0x85).

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

float LTR303Component::calculate_lux_(uint16_t channel0, uint16_t channel1, uint8_t gain, uint8_t time) {
  if ((channel0 == 0xFFFF) || (channel1 == 0xFFFF)) {
    ESP_LOGW(TAG, "Sensors got saturated");
    return 0.0;
  }

  if ((channel0 == 0x0000) && (channel1 == 0x0000)) {
    ESP_LOGW(TAG, "Sensors blacked out");
    return 0.0;
  }

  float ch0 = channel0;
  float ch1 = channel1;
  float ratio = ch1 / (ch0 + ch1);
  float als_gain = ALS_GAIN[gain];
  float als_int = ((float) ALS_INT_TIME[time]) / 100.0;
  float pfactor = this->attenuation_factor_;
  float lux = 0;

  ESP_LOGD(TAG, "CALC LUX ratio %f, gain %f, int %f, pfactor %f", ratio, als_gain, als_int, pfactor);

  if (ratio < 0.45) {
    lux = (1.7743 * ch0 + 1.1059 * ch1) / als_gain / als_int / pfactor;
  } else if (ratio < 0.64 && ratio >= 0.45) {
    lux = (4.2785 * ch0 - 1.9548 * ch1) / als_gain / als_int / pfactor;
  } else if (ratio < 0.85 && ratio >= 0.64) {
    lux = (0.5926 * ch0 + 0.1185 * ch1) / als_gain / als_int / pfactor;
  } else {
    ESP_LOGW(TAG, "Impossible ch1/(ch0 + ch1) ratio");
    lux = 0;
  }
  return lux;
}

bool LTR303Component::read_sensor_data_() {
  ESP_LOGD(TAG, "Reading sensor data");

  StatusRegister status{0};

  const uint32_t now = millis();
  while (true) {
    status.raw = this->reg(LTR303_ALS_STATUS).get();
    ESP_LOGD(TAG, "LTR303_ALS_STATUS = 0x%02X", status.raw);

    if (status.new_data) {
      ESP_LOGD(TAG, "New data is ready");
      break;
    }
    if (millis() - now > 100) {
      ESP_LOGW(TAG, "No data returned from sensor");
      return false;
    }
    ESP_LOGW(TAG, "Waiting for data");
    delay(5);
  }

  if (status.data_invalid) {
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
  this->channel_diff = (this->channel0 > this->channel1) ? this->channel0 - this->channel1 : 0;

  ESP_LOGD(TAG, "Channel data CH1 = %d, CH0 = %d, diff = %d", this->channel1, this->channel0, this->channel_diff);

  uint8_t actual_gain = status.gain;
  if (actual_gain != this->gain_) {
    ESP_LOGW(TAG, "Actual gain (%d) differs from requested (%d)", ALS_GAIN[actual_gain], ALS_GAIN[this->gain_]);
  }

  if (this->infrared_sensor_ != nullptr) {
    this->infrared_sensor_->publish_state(this->channel1);
  }

  if (this->full_spectrum_sensor_ != nullptr) {
    this->full_spectrum_sensor_->publish_state(this->channel0);
  }

  if (this->visible_sensor_ != nullptr) {
    this->visible_sensor_->publish_state(this->channel_diff);
  }

  if (this->actual_gain_sensor_ != nullptr) {
    this->actual_gain_sensor_->publish_state(ALS_GAIN[actual_gain]);
  }

  if (this->calculated_lux_sensor_ != nullptr) {
    float lux = this->calculate_lux_(this->channel0, this->channel1, actual_gain, this->integration_time_);
    ESP_LOGD(TAG, "Calculated lux: %f", lux);
    this->calculated_lux_sensor_->publish_state(lux);
  }

  return true;
}

void LTR303Component::reset_() {
  ESP_LOGD(TAG, "Resetting");

  ControlRegister als_ctrl{0};
  als_ctrl.sw_reset = true;
  this->reg(LTR303_ALS_CTRL) = als_ctrl.raw;
  delay(10);

  uint8_t tries = 10;
  do {
    ESP_LOGD(TAG, "Waiting chip to reset");
    delay(5);
    als_ctrl.raw = this->reg(LTR303_ALS_CTRL).get();
  } while (als_ctrl.sw_reset && tries--);  // while sw reset bit is on - keep waiting

  if (als_ctrl.sw_reset) {
    ESP_LOGW(TAG, "Failed to finalize reset procedure");
  }
}

void LTR303Component::activate_() {
  ESP_LOGD(TAG, "Activating");

  ControlRegister als_ctrl{0};
  als_ctrl.active_mode = true;
  als_ctrl.gain = this->gain_;

  ESP_LOGD(TAG, "Setting active mode and gain 0x%02X", als_ctrl.raw);
  this->reg(LTR303_ALS_CTRL) = als_ctrl.raw;
  delay(10);

  uint8_t tries = 10;
  do {
    ESP_LOGD(TAG, "Waiting chip to activate");
    delay(5);
    als_ctrl.raw = this->reg(LTR303_ALS_CTRL).get();
  } while (!als_ctrl.active_mode && tries--);  // while active mode is not set - keep waiting

  if (!als_ctrl.active_mode) {
    ESP_LOGW(TAG, "Failed to activate chip");
  }
}

void LTR303Component::configure_() {
  MeasurementRateRegister meas{0};
  meas.measurement_repeat_rate = this->repeat_rate_;
  meas.integration_time = this->integration_time_;
  ESP_LOGD(TAG, "Setting integration time and measurement repeat rate 0x%02X", meas.raw);
  this->reg(LTR303_MEAS_RATE) = meas.raw;
  delay(10);
}

void LTR303Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up LTR-303/329");
  delay(100);  // need to wait at least 100ms after power on to get ALS chip responsive
  App.feed_wdt();

  this->reset_();
  this->activate_();
  this->configure_();

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
  ESP_LOGCONFIG(TAG, "  Attenuation factor: %f", this->attenuation_factor_);
  LOG_UPDATE_INTERVAL(this);

  if (this->infrared_sensor_ != nullptr)
    LOG_SENSOR("  ", "CH1 Infrared", this->infrared_sensor_);
  if (this->full_spectrum_sensor_ != nullptr)
    LOG_SENSOR("  ", "CH0 Visible + IR", this->full_spectrum_sensor_);
  if (this->actual_gain_sensor_ != nullptr)
    LOG_SENSOR("  ", "Actrual gain", this->actual_gain_sensor_);
  if (this->calculated_lux_sensor_ != nullptr)
    LOG_SENSOR("  ", "CH0 Visible + IR", this->calculated_lux_sensor_);

  if (this->error_code_ == COMMUNICATION_FAILED) {
    ESP_LOGE(TAG, "Communication with I2C LTR-303/329 failed!");
  } else {
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
