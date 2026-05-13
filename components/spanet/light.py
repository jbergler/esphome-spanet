import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light
from esphome.const import CONF_OUTPUT_ID

from . import CONF_SPANET_ID, SpaNetComponent, spanet_ns

DEPENDENCIES = ["spanet"]

SpaNetLight = spanet_ns.class_("SpaNetLight", light.LightOutput, cg.Component)

CONFIG_SCHEMA = light.light_schema(SpaNetLight, light.LightType.RGB).extend(
    {
        cv.GenerateID(CONF_SPANET_ID): cv.use_id(SpaNetComponent),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_SPANET_ID])
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID], parent)
    await cg.register_component(var, config)
    await light.register_light(var, config)
