import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import CONF_ID

from . import CONF_SPANET_ID, SpaNetComponent, spanet_ns

DEPENDENCIES = ["spanet"]

CONF_LIGHT_EFFECT_SPEED = "light_effect_speed"
CONF_FILTRATION_HOURS = "filtration_hours"
CONF_FILTRATION_BLOCK_HOURS = "filtration_block_hours"

SpaNetLightEffectSpeedNumber = spanet_ns.class_(
    "SpaNetLightEffectSpeedNumber", number.Number, cg.Component
)
SpaNetFiltHrsNumber = spanet_ns.class_("SpaNetFiltHrsNumber", number.Number, cg.Component)
SpaNetFiltBlockHrsNumber = spanet_ns.class_("SpaNetFiltBlockHrsNumber", number.Number, cg.Component)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(CONF_SPANET_ID): cv.use_id(SpaNetComponent),
            cv.Optional(CONF_LIGHT_EFFECT_SPEED): number.number_schema(SpaNetLightEffectSpeedNumber),
            cv.Optional(CONF_FILTRATION_HOURS): number.number_schema(SpaNetFiltHrsNumber),
            cv.Optional(CONF_FILTRATION_BLOCK_HOURS): number.number_schema(SpaNetFiltBlockHrsNumber),
        }
    ),
    cv.has_at_least_one_key(
        CONF_LIGHT_EFFECT_SPEED, CONF_FILTRATION_HOURS, CONF_FILTRATION_BLOCK_HOURS
    ),
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_SPANET_ID])

    if CONF_LIGHT_EFFECT_SPEED in config:
        var = cg.new_Pvariable(config[CONF_LIGHT_EFFECT_SPEED][CONF_ID], parent)
        await cg.register_component(var, config[CONF_LIGHT_EFFECT_SPEED])
        await number.register_number(
            var, config[CONF_LIGHT_EFFECT_SPEED], min_value=1.0, max_value=5.0, step=1.0
        )

    if CONF_FILTRATION_HOURS in config:
        var = cg.new_Pvariable(config[CONF_FILTRATION_HOURS][CONF_ID], parent)
        await cg.register_component(var, config[CONF_FILTRATION_HOURS])
        await number.register_number(
            var, config[CONF_FILTRATION_HOURS], min_value=1.0, max_value=24.0, step=1.0
        )

    if CONF_FILTRATION_BLOCK_HOURS in config:
        var = cg.new_Pvariable(config[CONF_FILTRATION_BLOCK_HOURS][CONF_ID], parent)
        await cg.register_component(var, config[CONF_FILTRATION_BLOCK_HOURS])
        await number.register_number(
            var, config[CONF_FILTRATION_BLOCK_HOURS], min_value=1.0, max_value=24.0, step=1.0
        )
