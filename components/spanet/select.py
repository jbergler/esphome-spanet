import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from esphome.const import CONF_ID, ENTITY_CATEGORY_CONFIG

from . import CONF_SPANET_ID, SpaNetComponent, spanet_ns

DEPENDENCIES = ["spanet"]

CONF_LIGHT_EFFECT = "light_effect"
CONF_OPERATING_MODE = "operating_mode"

SpaNetLightEffectSelect = spanet_ns.class_("SpaNetLightEffectSelect", select.Select, cg.Component)
SpaNetOperatingModeSelect = spanet_ns.class_("SpaNetOperatingModeSelect", select.Select, cg.Component)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(CONF_SPANET_ID): cv.use_id(SpaNetComponent),
            cv.Optional(CONF_LIGHT_EFFECT): select.select_schema(SpaNetLightEffectSelect),
            cv.Optional(CONF_OPERATING_MODE): select.select_schema(SpaNetOperatingModeSelect, entity_category=ENTITY_CATEGORY_CONFIG),
        }
    ),
    cv.has_at_least_one_key(CONF_LIGHT_EFFECT, CONF_OPERATING_MODE),
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_SPANET_ID])

    if CONF_LIGHT_EFFECT in config:
        var = cg.new_Pvariable(config[CONF_LIGHT_EFFECT][CONF_ID], parent)
        await cg.register_component(var, config[CONF_LIGHT_EFFECT])
        await select.register_select(
            var, config[CONF_LIGHT_EFFECT], options=["White", "Colour", "Step", "Fade", "Party"]
        )

    if CONF_OPERATING_MODE in config:
        var = cg.new_Pvariable(config[CONF_OPERATING_MODE][CONF_ID], parent)
        await cg.register_component(var, config[CONF_OPERATING_MODE])
        await select.register_select(
            var, config[CONF_OPERATING_MODE], options=["Normal", "Economy", "Away", "Weekdays"]
        )
