import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import DEVICE_CLASS_TEMPERATURE, STATE_CLASS_MEASUREMENT, UNIT_CELSIUS

from . import CONF_SPANET_ID, SpaNetComponent

DEPENDENCIES = ["spanet"]

CONF_WATER_TEMPERATURE = "water_temperature"
CONF_SETPOINT_TEMPERATURE = "setpoint_temperature"

temperature_sensor_schema = sensor.sensor_schema(
    unit_of_measurement=UNIT_CELSIUS,
    accuracy_decimals=1,
    device_class=DEVICE_CLASS_TEMPERATURE,
    state_class=STATE_CLASS_MEASUREMENT,
)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(CONF_SPANET_ID): cv.use_id(SpaNetComponent),
            cv.Optional(CONF_WATER_TEMPERATURE): temperature_sensor_schema,
            cv.Optional(CONF_SETPOINT_TEMPERATURE): temperature_sensor_schema,
        }
    ),
    cv.has_at_least_one_key(CONF_WATER_TEMPERATURE, CONF_SETPOINT_TEMPERATURE),
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_SPANET_ID])

    if CONF_WATER_TEMPERATURE in config:
        water = await sensor.new_sensor(config[CONF_WATER_TEMPERATURE])
        cg.add(parent.set_water_temperature_sensor(water))

    if CONF_SETPOINT_TEMPERATURE in config:
        setpoint = await sensor.new_sensor(config[CONF_SETPOINT_TEMPERATURE])
        cg.add(parent.set_setpoint_temperature_sensor(setpoint))
