import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID, ENTITY_CATEGORY_DIAGNOSTIC

from . import CONF_SPANET_ID, SpaNetComponent, spanet_ns

DEPENDENCIES = ["spanet"]

CONF_CONTROLLER_MODEL = "controller_model"
CONF_CONTROLLER_SERIAL = "controller_serial"
CONF_CONTROLLER_FW_VERSION = "controller_fw_version"
CONF_CURRENT_TIME = "current_time"

SpaNetTextSensor = spanet_ns.class_("SpaNetTextSensor", text_sensor.TextSensor, cg.Component)
TextSensorKind = spanet_ns.enum("TextSensorKind")

controller_text_sensor_schema = text_sensor.text_sensor_schema(
    SpaNetTextSensor,
    entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(CONF_SPANET_ID): cv.use_id(SpaNetComponent),
            cv.Optional(CONF_CONTROLLER_MODEL): controller_text_sensor_schema,
            cv.Optional(CONF_CONTROLLER_SERIAL): controller_text_sensor_schema,
            cv.Optional(CONF_CONTROLLER_FW_VERSION): controller_text_sensor_schema,
            cv.Optional(CONF_CURRENT_TIME): controller_text_sensor_schema,
        }
    ),
    cv.has_at_least_one_key(
        CONF_CONTROLLER_MODEL,
        CONF_CONTROLLER_SERIAL,
        CONF_CONTROLLER_FW_VERSION,
        CONF_CURRENT_TIME,
    ),
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_SPANET_ID])

    if CONF_CONTROLLER_MODEL in config:
        var = cg.new_Pvariable(config[CONF_CONTROLLER_MODEL][CONF_ID], parent, TextSensorKind.kControllerModel)
        await cg.register_component(var, config[CONF_CONTROLLER_MODEL])
        await text_sensor.register_text_sensor(var, config[CONF_CONTROLLER_MODEL])

    if CONF_CONTROLLER_SERIAL in config:
        var = cg.new_Pvariable(config[CONF_CONTROLLER_SERIAL][CONF_ID], parent, TextSensorKind.kControllerSerial)
        await cg.register_component(var, config[CONF_CONTROLLER_SERIAL])
        await text_sensor.register_text_sensor(var, config[CONF_CONTROLLER_SERIAL])

    if CONF_CONTROLLER_FW_VERSION in config:
        var = cg.new_Pvariable(config[CONF_CONTROLLER_FW_VERSION][CONF_ID], parent, TextSensorKind.kControllerFwVersion)
        await cg.register_component(var, config[CONF_CONTROLLER_FW_VERSION])
        await text_sensor.register_text_sensor(var, config[CONF_CONTROLLER_FW_VERSION])

    if CONF_CURRENT_TIME in config:
        var = cg.new_Pvariable(config[CONF_CURRENT_TIME][CONF_ID], parent, TextSensorKind.kCurrentTime)
        await cg.register_component(var, config[CONF_CURRENT_TIME])
        await text_sensor.register_text_sensor(var, config[CONF_CURRENT_TIME])
