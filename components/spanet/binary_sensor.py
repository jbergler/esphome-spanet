import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_HEAT,
    DEVICE_CLASS_MOISTURE,
    DEVICE_CLASS_RUNNING,
)

from . import CONF_SPANET_ID, SpaNetComponent, spanet_ns

DEPENDENCIES = ["spanet"]

CONF_HEATING_ACTIVE = "heating_active"
CONF_OZONE_ACTIVE = "ozone_active"
CONF_CLEAN_CYCLE_ACTIVE = "clean_cycle_active"
CONF_WATER_PRESENT = "water_present"

SpaNetBinarySensor = spanet_ns.class_("SpaNetBinarySensor", binary_sensor.BinarySensor, cg.Component)
BinarySensorKind = spanet_ns.enum("BinarySensorKind")

heating_active_schema = binary_sensor.binary_sensor_schema(
    SpaNetBinarySensor,
    device_class=DEVICE_CLASS_HEAT,
)

ozone_active_schema = binary_sensor.binary_sensor_schema(
    SpaNetBinarySensor,
    device_class=DEVICE_CLASS_RUNNING,
)

clean_cycle_active_schema = binary_sensor.binary_sensor_schema(
    SpaNetBinarySensor,
    device_class=DEVICE_CLASS_RUNNING,
)

water_present_schema = binary_sensor.binary_sensor_schema(
    SpaNetBinarySensor,
    device_class=DEVICE_CLASS_MOISTURE,
)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(CONF_SPANET_ID): cv.use_id(SpaNetComponent),
            cv.Optional(CONF_HEATING_ACTIVE): heating_active_schema,
            cv.Optional(CONF_OZONE_ACTIVE): ozone_active_schema,
            cv.Optional(CONF_CLEAN_CYCLE_ACTIVE): clean_cycle_active_schema,
            cv.Optional(CONF_WATER_PRESENT): water_present_schema,
        }
    ),
    cv.has_at_least_one_key(
        CONF_HEATING_ACTIVE,
        CONF_OZONE_ACTIVE,
        CONF_CLEAN_CYCLE_ACTIVE,
        CONF_WATER_PRESENT,
    ),
)

_KIND_MAP = {
    CONF_HEATING_ACTIVE: "kHeatingActive",
    CONF_OZONE_ACTIVE: "kOzoneActive",
    CONF_CLEAN_CYCLE_ACTIVE: "kCleanCycleActive",
    CONF_WATER_PRESENT: "kWaterPresent",
}


async def to_code(config):
    parent = await cg.get_variable(config[CONF_SPANET_ID])

    for conf_key, kind_name in _KIND_MAP.items():
        if conf_key not in config:
            continue
        var = cg.new_Pvariable(
            config[conf_key][CONF_ID], parent, getattr(BinarySensorKind, kind_name)
        )
        await cg.register_component(var, config[conf_key])
        await binary_sensor.register_binary_sensor(var, config[conf_key])
