import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c, sensor
from esphome.const import (
    CONF_ID,
    CONF_INTEGRATION_TIME,
    CONF_REPEAT,
    CONF_GLASS_ATTENUATION_FACTOR,
    CONF_GAIN,
    CONF_ACTUAL_GAIN,
    CONF_CALCULATED_LUX,
    CONF_FULL_SPECTRUM,
    CONF_INFRARED,
    UNIT_LUX,
    ICON_BRIGHTNESS_6,
    DEVICE_CLASS_ILLUMINANCE,
    STATE_CLASS_MEASUREMENT
)

UNIT_COUNTS = "counts"

CODEOWNERS = ["@latonita"]
DEPENDENCIES = ["i2c"]

ltr303_ns = cg.esphome_ns.namespace("ltr303")

LTR303Component = ltr303_ns.class_(
    "LTR303Component", cg.PollingComponent, i2c.I2CDevice
)

LTR303Gain = ltr303_ns.enum("LTR303Gain")
GAINS = {
    "X1": LTR303Gain.LTR303_GAIN_1,
    "X2": LTR303Gain.LTR303_GAIN_2,
    "X4": LTR303Gain.LTR303_GAIN_4,
    "X8": LTR303Gain.LTR303_GAIN_8,
    "X48": LTR303Gain.LTR303_GAIN_48,
    "X96": LTR303Gain.LTR303_GAIN_96,
}

LTR303IntegrationTime = ltr303_ns.enum("LTR303IntegrationTime")
INTEGRATION_TIMES = {
    50: LTR303IntegrationTime.LTR303_INTEGRATION_TIME_50MS,
    100: LTR303IntegrationTime.LTR303_INTEGRATION_TIME_100MS,
    150: LTR303IntegrationTime.LTR303_INTEGRATION_TIME_150MS,
    200: LTR303IntegrationTime.LTR303_INTEGRATION_TIME_200MS,
    250: LTR303IntegrationTime.LTR303_INTEGRATION_TIME_250MS,
    300: LTR303IntegrationTime.LTR303_INTEGRATION_TIME_300MS,
    350: LTR303IntegrationTime.LTR303_INTEGRATION_TIME_350MS,
    400: LTR303IntegrationTime.LTR303_INTEGRATION_TIME_400MS,
}

LTR303MeasurementRepeatRate = ltr303_ns.enum("LTR303MeasurementRepeatRate")
MEASUREMENT_REPEAT_RATES = {
    50: LTR303MeasurementRepeatRate.LTR303_MEASUREMENT_REPEAT_RATE_50MS,
    100: LTR303MeasurementRepeatRate.LTR303_MEASUREMENT_REPEAT_RATE_100MS,
    200: LTR303MeasurementRepeatRate.LTR303_MEASUREMENT_REPEAT_RATE_200MS,
    500: LTR303MeasurementRepeatRate.LTR303_MEASUREMENT_REPEAT_RATE_500MS,
    1000: LTR303MeasurementRepeatRate.LTR303_MEASUREMENT_REPEAT_RATE_1000MS,
    2000: LTR303MeasurementRepeatRate.LTR303_MEASUREMENT_REPEAT_RATE_2000MS,
}

def validate_integration_time(value):
    value = cv.positive_time_period_milliseconds(value).total_milliseconds
    return cv.enum(INTEGRATION_TIMES, int=True)(value)

def validate_repeat_rate(value):
    value = cv.positive_time_period_milliseconds(value).total_milliseconds
    return cv.enum(MEASUREMENT_REPEAT_RATES, int=True)(value)

def validate_time_and_repeat_rate(config):
    integraton_time = config[CONF_INTEGRATION_TIME]
    repeat_rate = config[CONF_REPEAT]
    if (integraton_time > repeat_rate):
        raise cv.Invalid(f"Measurement repeat rate ({repeat_rate}ms) shall be greater or equal to integration time ({integraton_time}ms)")
    return config

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(LTR303Component),
            cv.Optional(CONF_INFRARED): sensor.sensor_schema(
                unit_of_measurement=UNIT_COUNTS,
                icon=ICON_BRIGHTNESS_6,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_ILLUMINANCE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_FULL_SPECTRUM): sensor.sensor_schema(
                unit_of_measurement=UNIT_COUNTS,
                icon=ICON_BRIGHTNESS_6,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_ILLUMINANCE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_ACTUAL_GAIN): sensor.sensor_schema(
                icon=ICON_BRIGHTNESS_6,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_ILLUMINANCE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_CALCULATED_LUX): sensor.sensor_schema(
                unit_of_measurement=UNIT_LUX,
                icon=ICON_BRIGHTNESS_6,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_ILLUMINANCE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_GAIN, default="X1"): cv.enum(GAINS),
            cv.Optional(CONF_INTEGRATION_TIME, default="100ms"): validate_integration_time,
            cv.Optional(CONF_REPEAT, default="500ms"): validate_repeat_rate,
            cv.Optional(CONF_GLASS_ATTENUATION_FACTOR, default=1.0): cv.float_range(min = 1.0),
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(i2c.i2c_device_schema(0x29)),
    cv.has_at_least_one_key(CONF_INFRARED, CONF_FULL_SPECTRUM, CONF_CALCULATED_LUX, CONF_ACTUAL_GAIN),
    validate_time_and_repeat_rate,
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    if CONF_INFRARED in config:
        conf = config[CONF_INFRARED]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_infrared_sensor(sens))

    if CONF_FULL_SPECTRUM in config:
        conf = config[CONF_FULL_SPECTRUM]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_full_spectrum_sensor(sens))

    if CONF_ACTUAL_GAIN in config:
        conf = config[CONF_ACTUAL_GAIN]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_actual_gain_sensor(sens))

    if CONF_CALCULATED_LUX in config:
        conf = config[CONF_CALCULATED_LUX]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_calculated_lux_sensor(sens))

    cg.add(var.set_gain_value(config[CONF_GAIN]))
    cg.add(var.set_integration_time_value(config[CONF_INTEGRATION_TIME]))
    cg.add(var.set_repeat_rate_value(config[CONF_REPEAT]))
    cg.add(var.set_attenuation_factor(config[CONF_GLASS_ATTENUATION_FACTOR]))
