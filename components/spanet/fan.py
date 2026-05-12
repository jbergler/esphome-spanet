import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import fan
from esphome.const import CONF_ID

from . import CONF_SPANET_ID, SpaNetComponent, spanet_ns

DEPENDENCIES = ["spanet"]

CONF_PUMP = "pump"

SpaNetPumpFan = spanet_ns.class_("SpaNetPumpFan", fan.Fan, cg.Component)

CONFIG_SCHEMA = fan.fan_schema(SpaNetPumpFan).extend(
    {
        cv.GenerateID(CONF_SPANET_ID): cv.use_id(SpaNetComponent),
        cv.Required(CONF_PUMP): cv.int_range(min=1, max=5),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_SPANET_ID])
    var = cg.new_Pvariable(config[CONF_ID], parent, config[CONF_PUMP])
    await cg.register_component(var, config)
    await fan.register_fan(var, config)
