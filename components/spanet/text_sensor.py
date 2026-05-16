import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import ENTITY_CATEGORY_DIAGNOSTIC

from . import CONF_SPANET_ID, SpaNetComponent

DEPENDENCIES = ["spanet"]

CONF_CONTROLLER_MODEL = "controller_model"
CONF_CONTROLLER_SERIAL = "controller_serial"
CONF_CONTROLLER_FW_VERSION = "controller_fw_version"
CONF_CURRENT_TIME = "current_time"

controller_text_sensor_schema = text_sensor.text_sensor_schema(
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
        model = await text_sensor.new_text_sensor(config[CONF_CONTROLLER_MODEL])
        cg.add(parent.set_controller_model_sensor(model))

    if CONF_CONTROLLER_SERIAL in config:
        serial = await text_sensor.new_text_sensor(config[CONF_CONTROLLER_SERIAL])
        cg.add(parent.set_controller_serial_sensor(serial))

    if CONF_CONTROLLER_FW_VERSION in config:
        fw_version = await text_sensor.new_text_sensor(config[CONF_CONTROLLER_FW_VERSION])
        cg.add(parent.set_controller_fw_version_sensor(fw_version))

    if CONF_CURRENT_TIME in config:
        current_time = await text_sensor.new_text_sensor(config[CONF_CURRENT_TIME])
        cg.add(parent.set_current_time_sensor(current_time))
