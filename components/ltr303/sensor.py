import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import i2c, sensor
from esphome.const import (
    CONF_ID,
    CONF_ACTUAL_GAIN,
    CONF_AUTO_MODE,
    CONF_GAIN,
    CONF_GLASS_ATTENUATION_FACTOR,
    CONF_INTEGRATION_TIME,
    CONF_REPEAT,
    CONF_TRIGGER_ID,
    UNIT_LUX,
    UNIT_MILLISECOND,
    ICON_BRIGHTNESS_5,
    ICON_BRIGHTNESS_6,
    ICON_TIMER,
    DEVICE_CLASS_ILLUMINANCE,
    DEVICE_CLASS_DISTANCE,
    STATE_CLASS_MEASUREMENT,
)

CODEOWNERS = ["@latonita"]
DEPENDENCIES = ["i2c"]

UNIT_COUNTS = "#"
ICON_GAIN = "mdi:multiplication"
ICON_BRIGHTNESS_7 = "mdi:brightness-7"
ICON_PROXIMITY = "mdi:hand-wave-outline"
CONF_ACTUAL_INTEGRATION_TIME = "actual_integration_time"
CONF_AMBIENT_LIGHT = "ambient_light"
CONF_ENABLE_PROXIMITY = "enable_proximity"
CONF_FULL_SPECTRUM_COUNTS = "full_spectrum_counts"
CONF_INFRARED_COUNTS = "infrared_counts"
CONF_PROXIMITY_COUNTS = "proximity_counts"
CONF_PROXIMITY_HIGH_THRESHOLD = "proximity_high_threshold"
CONF_PROXIMITY_LOW_THRESHOLD = "proximity_low_threshold"
CONF_PROXIMITY_COOLDOWN = "proximity_cooldown"
CONF_ON_HIGH_THRESHOLD = "on_high_threshold"
CONF_ON_LOW_THRESHOLD = "on_low_threshold"

ltr303_ns = cg.esphome_ns.namespace("ltr303")

LTR303Component = ltr303_ns.class_(
    "LTR303Component", cg.PollingComponent, i2c.I2CDevice
)

LTR303HighTrigger = ltr303_ns.class_("LTR303HighTrigger", automation.Trigger.template())

LTR303LowTrigger = ltr303_ns.class_("LTR303LowTrigger", automation.Trigger.template())

Gain = ltr303_ns.enum("Gain")
GAINS = {
    "1X": Gain.GAIN_1,
    "2X": Gain.GAIN_2,
    "4X": Gain.GAIN_4,
    "8X": Gain.GAIN_8,
    "48X": Gain.GAIN_48,
    "96X": Gain.GAIN_96,
}

IntegrationTime = ltr303_ns.enum("IntegrationTime")
INTEGRATION_TIMES = {
    50: IntegrationTime.INTEGRATION_TIME_50MS,
    100: IntegrationTime.INTEGRATION_TIME_100MS,
    150: IntegrationTime.INTEGRATION_TIME_150MS,
    200: IntegrationTime.INTEGRATION_TIME_200MS,
    250: IntegrationTime.INTEGRATION_TIME_250MS,
    300: IntegrationTime.INTEGRATION_TIME_300MS,
    350: IntegrationTime.INTEGRATION_TIME_350MS,
    400: IntegrationTime.INTEGRATION_TIME_400MS,
}

MeasurementRepeatRate = ltr303_ns.enum("MeasurementRepeatRate")
MEASUREMENT_REPEAT_RATES = {
    50: MeasurementRepeatRate.REPEAT_RATE_50MS,
    100: MeasurementRepeatRate.REPEAT_RATE_100MS,
    200: MeasurementRepeatRate.REPEAT_RATE_200MS,
    500: MeasurementRepeatRate.REPEAT_RATE_500MS,
    1000: MeasurementRepeatRate.REPEAT_RATE_1000MS,
    2000: MeasurementRepeatRate.REPEAT_RATE_2000MS,
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
    if integraton_time > repeat_rate:
        raise cv.Invalid(
            f"Measurement repeat rate ({repeat_rate}ms) shall be greater or equal to integration time ({integraton_time}ms)"
        )
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(LTR303Component),
            cv.Optional(CONF_AUTO_MODE, default=True): cv.boolean,
            cv.Optional(CONF_GAIN, default="1X"): cv.enum(GAINS, upper=True),
            cv.Optional(
                CONF_INTEGRATION_TIME, default="100ms"
            ): validate_integration_time,
            cv.Optional(CONF_REPEAT, default="500ms"): validate_repeat_rate,
            cv.Optional(CONF_GLASS_ATTENUATION_FACTOR, default=1.0): cv.float_range(
                min=1.0
            ),
            cv.Optional(CONF_ENABLE_PROXIMITY, default=False): cv.boolean,
            cv.Optional(CONF_ON_HIGH_THRESHOLD): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(LTR303HighTrigger),
                }
            ),
            cv.Optional(CONF_ON_LOW_THRESHOLD): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(LTR303LowTrigger),
                }
            ),
            cv.Optional(
                CONF_PROXIMITY_COOLDOWN, default="5s"
            ): cv.positive_time_period_seconds,
            cv.Optional(CONF_PROXIMITY_HIGH_THRESHOLD, default=65535): cv.int_range(
                min=0, max=65535
            ),
            cv.Optional(CONF_PROXIMITY_LOW_THRESHOLD, default=0): cv.int_range(
                min=0, max=65535
            ),
            cv.Optional(CONF_AMBIENT_LIGHT): sensor.sensor_schema(
                unit_of_measurement=UNIT_LUX,
                icon=ICON_BRIGHTNESS_6,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_ILLUMINANCE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_INFRARED_COUNTS): sensor.sensor_schema(
                unit_of_measurement=UNIT_COUNTS,
                icon=ICON_BRIGHTNESS_5,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_ILLUMINANCE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_FULL_SPECTRUM_COUNTS): sensor.sensor_schema(
                unit_of_measurement=UNIT_COUNTS,
                icon=ICON_BRIGHTNESS_7,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_ILLUMINANCE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_PROXIMITY_COUNTS): sensor.sensor_schema(
                unit_of_measurement=UNIT_COUNTS,
                icon=ICON_PROXIMITY,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_DISTANCE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_ACTUAL_GAIN): sensor.sensor_schema(
                icon=ICON_GAIN,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_ILLUMINANCE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_ACTUAL_INTEGRATION_TIME): sensor.sensor_schema(
                unit_of_measurement=UNIT_MILLISECOND,
                icon=ICON_TIMER,
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(i2c.i2c_device_schema(0x29)),
    validate_time_and_repeat_rate,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    if als_config := config.get(CONF_AMBIENT_LIGHT):
        sens = await sensor.new_sensor(als_config)
        cg.add(var.set_ambient_light_sensor(sens))

    if infrared_cnt_config := config.get(CONF_INFRARED_COUNTS):
        sens = await sensor.new_sensor(infrared_cnt_config)
        cg.add(var.set_infrared_counts_sensor(sens))

    if full_spect_cnt_config := config.get(CONF_FULL_SPECTRUM_COUNTS):
        sens = await sensor.new_sensor(full_spect_cnt_config)
        cg.add(var.set_full_spectrum_counts_sensor(sens))

    if act_gain_config := config.get(CONF_ACTUAL_GAIN):
        sens = await sensor.new_sensor(act_gain_config)
        cg.add(var.set_actual_gain_sensor(sens))

    if act_itime_config := config.get(CONF_ACTUAL_INTEGRATION_TIME):
        sens = await sensor.new_sensor(act_itime_config)
        cg.add(var.set_actual_integration_time_sensor(sens))

    if prox_cnt_config := config.get(CONF_PROXIMITY_COUNTS):
        sens = await sensor.new_sensor(prox_cnt_config)
        cg.add(var.set_proximity_counts_sensor(sens))

    for prox_high_tr in config.get(CONF_ON_HIGH_THRESHOLD, []):
        trigger = cg.new_Pvariable(prox_high_tr[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], prox_high_tr)

    for prox_low_tr in config.get(CONF_ON_LOW_THRESHOLD, []):
        trigger = cg.new_Pvariable(prox_low_tr[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], prox_low_tr)

    cg.add(var.set_enable_automatic_mode(config[CONF_AUTO_MODE]))
    cg.add(var.set_enable_proximity_mode(config[CONF_ENABLE_PROXIMITY]))
    cg.add(var.set_gain(config[CONF_GAIN]))
    cg.add(var.set_integration_time(config[CONF_INTEGRATION_TIME]))
    cg.add(var.set_repeat_rate(config[CONF_REPEAT]))
    cg.add(var.set_glass_attenuation_factor(config[CONF_GLASS_ATTENUATION_FACTOR]))
    cg.add(var.set_proximity_high_threshold(config[CONF_PROXIMITY_HIGH_THRESHOLD]))
    cg.add(var.set_proximity_low_threshold(config[CONF_PROXIMITY_LOW_THRESHOLD]))
    cg.add(var.set_proximity_cooldown_time_s(config[CONF_PROXIMITY_COOLDOWN]))
