import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor

from . import CONF_SPANET_ID, SpaNetComponent

DEPENDENCIES = ["spanet"]

CONFIG_SCHEMA = text_sensor.text_sensor_schema().extend(
    {
        cv.GenerateID(CONF_SPANET_ID): cv.use_id(SpaNetComponent),
    }
)


async def to_code(config):
    controller = await text_sensor.new_text_sensor(config)
    parent = await cg.get_variable(config[CONF_SPANET_ID])
    cg.add(parent.set_model_sensor(controller))
