import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import CONF_ID

from . import CONF_SPANET_ID, SpaNetComponent, spanet_ns

DEPENDENCIES = ["spanet"]

SpaNetLightEffectSpeedNumber = spanet_ns.class_(
    "SpaNetLightEffectSpeedNumber", number.Number, cg.Component
)

CONFIG_SCHEMA = number.number_schema(SpaNetLightEffectSpeedNumber).extend(
    {
        cv.GenerateID(CONF_SPANET_ID): cv.use_id(SpaNetComponent),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_SPANET_ID])
    var = cg.new_Pvariable(config[CONF_ID], parent)
    await cg.register_component(var, config)
    await number.register_number(var, config, min_value=1.0, max_value=5.0, step=1.0)
