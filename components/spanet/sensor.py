import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_ENERGY,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_VOLTAGE,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_AMPERE,
    UNIT_CELSIUS,
    UNIT_KILOWATT_HOURS,
    UNIT_VOLT,
    UNIT_WATT,
)

from . import CONF_SPANET_ID, SpaNetComponent, spanet_ns

DEPENDENCIES = ["spanet"]

CONF_WATER_TEMPERATURE = "water_temperature"
CONF_SETPOINT_TEMPERATURE = "setpoint_temperature"
CONF_HEATER_TEMPERATURE = "heater_temperature"
CONF_CASE_TEMPERATURE = "case_temperature"
CONF_MAINS_VOLTAGE = "mains_voltage"
CONF_MAINS_CURRENT = "mains_current"
CONF_INSTANT_POWER = "instant_power"
CONF_TOTAL_ENERGY = "total_energy"

SpaNetSensor = spanet_ns.class_("SpaNetSensor", sensor.Sensor, cg.Component)
SensorKind = spanet_ns.enum("SensorKind")

temperature_sensor_schema = sensor.sensor_schema(
    SpaNetSensor,
    unit_of_measurement=UNIT_CELSIUS,
    accuracy_decimals=1,
    device_class=DEVICE_CLASS_TEMPERATURE,
    state_class=STATE_CLASS_MEASUREMENT,
)

diag_temperature_sensor_schema = sensor.sensor_schema(
    SpaNetSensor,
    unit_of_measurement=UNIT_CELSIUS,
    accuracy_decimals=1,
    device_class=DEVICE_CLASS_TEMPERATURE,
    state_class=STATE_CLASS_MEASUREMENT,
    entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
)

voltage_sensor_schema = sensor.sensor_schema(
    SpaNetSensor,
    unit_of_measurement=UNIT_VOLT,
    accuracy_decimals=0,
    device_class=DEVICE_CLASS_VOLTAGE,
    state_class=STATE_CLASS_MEASUREMENT,
    entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
)

current_sensor_schema = sensor.sensor_schema(
    SpaNetSensor,
    unit_of_measurement=UNIT_AMPERE,
    accuracy_decimals=1,
    device_class=DEVICE_CLASS_CURRENT,
    state_class=STATE_CLASS_MEASUREMENT,
    entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
)

power_sensor_schema = sensor.sensor_schema(
    SpaNetSensor,
    unit_of_measurement=UNIT_WATT,
    accuracy_decimals=1,
    device_class=DEVICE_CLASS_POWER,
    state_class=STATE_CLASS_MEASUREMENT,
    entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
)

energy_sensor_schema = sensor.sensor_schema(
    SpaNetSensor,
    unit_of_measurement=UNIT_KILOWATT_HOURS,
    accuracy_decimals=2,
    device_class=DEVICE_CLASS_ENERGY,
    state_class=STATE_CLASS_TOTAL_INCREASING,
    entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(CONF_SPANET_ID): cv.use_id(SpaNetComponent),
            cv.Optional(CONF_WATER_TEMPERATURE): temperature_sensor_schema,
            cv.Optional(CONF_SETPOINT_TEMPERATURE): temperature_sensor_schema,
            cv.Optional(CONF_HEATER_TEMPERATURE): diag_temperature_sensor_schema,
            cv.Optional(CONF_CASE_TEMPERATURE): diag_temperature_sensor_schema,
            cv.Optional(CONF_MAINS_VOLTAGE): voltage_sensor_schema,
            cv.Optional(CONF_MAINS_CURRENT): current_sensor_schema,
            cv.Optional(CONF_INSTANT_POWER): power_sensor_schema,
            cv.Optional(CONF_TOTAL_ENERGY): energy_sensor_schema,
        }
    ),
    cv.has_at_least_one_key(
        CONF_WATER_TEMPERATURE,
        CONF_SETPOINT_TEMPERATURE,
        CONF_HEATER_TEMPERATURE,
        CONF_CASE_TEMPERATURE,
        CONF_MAINS_VOLTAGE,
        CONF_MAINS_CURRENT,
        CONF_INSTANT_POWER,
        CONF_TOTAL_ENERGY,
    ),
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_SPANET_ID])

    if CONF_WATER_TEMPERATURE in config:
        var = cg.new_Pvariable(config[CONF_WATER_TEMPERATURE][CONF_ID], parent, SensorKind.kWaterTemperature)
        await cg.register_component(var, config[CONF_WATER_TEMPERATURE])
        await sensor.register_sensor(var, config[CONF_WATER_TEMPERATURE])

    if CONF_SETPOINT_TEMPERATURE in config:
        var = cg.new_Pvariable(config[CONF_SETPOINT_TEMPERATURE][CONF_ID], parent, SensorKind.kSetpointTemperature)
        await cg.register_component(var, config[CONF_SETPOINT_TEMPERATURE])
        await sensor.register_sensor(var, config[CONF_SETPOINT_TEMPERATURE])

    if CONF_HEATER_TEMPERATURE in config:
        var = cg.new_Pvariable(config[CONF_HEATER_TEMPERATURE][CONF_ID], parent, SensorKind.kHeaterTemperature)
        await cg.register_component(var, config[CONF_HEATER_TEMPERATURE])
        await sensor.register_sensor(var, config[CONF_HEATER_TEMPERATURE])

    if CONF_CASE_TEMPERATURE in config:
        var = cg.new_Pvariable(config[CONF_CASE_TEMPERATURE][CONF_ID], parent, SensorKind.kCaseTemperature)
        await cg.register_component(var, config[CONF_CASE_TEMPERATURE])
        await sensor.register_sensor(var, config[CONF_CASE_TEMPERATURE])

    if CONF_MAINS_VOLTAGE in config:
        var = cg.new_Pvariable(config[CONF_MAINS_VOLTAGE][CONF_ID], parent, SensorKind.kMainsVoltage)
        await cg.register_component(var, config[CONF_MAINS_VOLTAGE])
        await sensor.register_sensor(var, config[CONF_MAINS_VOLTAGE])

    if CONF_MAINS_CURRENT in config:
        var = cg.new_Pvariable(config[CONF_MAINS_CURRENT][CONF_ID], parent, SensorKind.kMainsCurrent)
        await cg.register_component(var, config[CONF_MAINS_CURRENT])
        await sensor.register_sensor(var, config[CONF_MAINS_CURRENT])

    if CONF_INSTANT_POWER in config:
        var = cg.new_Pvariable(config[CONF_INSTANT_POWER][CONF_ID], parent, SensorKind.kInstantPower)
        await cg.register_component(var, config[CONF_INSTANT_POWER])
        await sensor.register_sensor(var, config[CONF_INSTANT_POWER])

    if CONF_TOTAL_ENERGY in config:
        var = cg.new_Pvariable(config[CONF_TOTAL_ENERGY][CONF_ID], parent, SensorKind.kTotalEnergy)
        await cg.register_component(var, config[CONF_TOTAL_ENERGY])
        await sensor.register_sensor(var, config[CONF_TOTAL_ENERGY])
