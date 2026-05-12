import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate
from esphome.const import CONF_ID

from . import CONF_SPANET_ID, SpaNetComponent, spanet_ns

DEPENDENCIES = ["spanet"]

SpaNetClimate = spanet_ns.class_("SpaNetClimate", climate.Climate, cg.Component)

CONFIG_SCHEMA = climate.climate_schema(SpaNetClimate).extend(
    {
        cv.GenerateID(CONF_SPANET_ID): cv.use_id(SpaNetComponent),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_SPANET_ID])
    var = cg.new_Pvariable(config[CONF_ID], parent)
    await cg.register_component(var, config)
    await climate.register_climate(var, config)
