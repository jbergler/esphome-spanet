import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import CONF_ID, ENTITY_CATEGORY_CONFIG

from . import CONF_SPANET_ID, SpaNetComponent, spanet_ns

DEPENDENCIES = ["spanet"]

CONF_LIGHT_EFFECT_SPEED = "light_effect_speed"
CONF_FILTRATION_HOURS = "filtration_hours"
CONF_FILTRATION_BLOCK_HOURS = "filtration_block_hours"
CONF_SLEEP_TIMER_1_BEGIN_HOUR = "sleep_timer_1_begin_hour"
CONF_SLEEP_TIMER_1_BEGIN_MINUTE = "sleep_timer_1_begin_minute"
CONF_SLEEP_TIMER_1_END_HOUR = "sleep_timer_1_end_hour"
CONF_SLEEP_TIMER_1_END_MINUTE = "sleep_timer_1_end_minute"
CONF_SLEEP_TIMER_2_BEGIN_HOUR = "sleep_timer_2_begin_hour"
CONF_SLEEP_TIMER_2_BEGIN_MINUTE = "sleep_timer_2_begin_minute"
CONF_SLEEP_TIMER_2_END_HOUR = "sleep_timer_2_end_hour"
CONF_SLEEP_TIMER_2_END_MINUTE = "sleep_timer_2_end_minute"

SpaNetLightEffectSpeedNumber = spanet_ns.class_(
    "SpaNetLightEffectSpeedNumber", number.Number, cg.Component
)
SpaNetFiltHrsNumber = spanet_ns.class_("SpaNetFiltHrsNumber", number.Number, cg.Component)
SpaNetFiltBlockHrsNumber = spanet_ns.class_("SpaNetFiltBlockHrsNumber", number.Number, cg.Component)
SpaNetSleepTimerTimeNumber = spanet_ns.class_(
    "SpaNetSleepTimerTimeNumber", number.Number, cg.Component
)
SleepTimerEndpoint = spanet_ns.enum("SleepTimerEndpoint")
SleepTimerField = spanet_ns.enum("SleepTimerField")

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(CONF_SPANET_ID): cv.use_id(SpaNetComponent),
            cv.Optional(CONF_LIGHT_EFFECT_SPEED): number.number_schema(SpaNetLightEffectSpeedNumber),
            cv.Optional(CONF_FILTRATION_HOURS): number.number_schema(SpaNetFiltHrsNumber, entity_category=ENTITY_CATEGORY_CONFIG),
            cv.Optional(CONF_FILTRATION_BLOCK_HOURS): number.number_schema(SpaNetFiltBlockHrsNumber, entity_category=ENTITY_CATEGORY_CONFIG),
            cv.Optional(CONF_SLEEP_TIMER_1_BEGIN_HOUR): number.number_schema(SpaNetSleepTimerTimeNumber, entity_category=ENTITY_CATEGORY_CONFIG),
            cv.Optional(CONF_SLEEP_TIMER_1_BEGIN_MINUTE): number.number_schema(SpaNetSleepTimerTimeNumber, entity_category=ENTITY_CATEGORY_CONFIG),
            cv.Optional(CONF_SLEEP_TIMER_1_END_HOUR): number.number_schema(SpaNetSleepTimerTimeNumber, entity_category=ENTITY_CATEGORY_CONFIG),
            cv.Optional(CONF_SLEEP_TIMER_1_END_MINUTE): number.number_schema(SpaNetSleepTimerTimeNumber, entity_category=ENTITY_CATEGORY_CONFIG),
            cv.Optional(CONF_SLEEP_TIMER_2_BEGIN_HOUR): number.number_schema(SpaNetSleepTimerTimeNumber, entity_category=ENTITY_CATEGORY_CONFIG),
            cv.Optional(CONF_SLEEP_TIMER_2_BEGIN_MINUTE): number.number_schema(SpaNetSleepTimerTimeNumber, entity_category=ENTITY_CATEGORY_CONFIG),
            cv.Optional(CONF_SLEEP_TIMER_2_END_HOUR): number.number_schema(SpaNetSleepTimerTimeNumber, entity_category=ENTITY_CATEGORY_CONFIG),
            cv.Optional(CONF_SLEEP_TIMER_2_END_MINUTE): number.number_schema(SpaNetSleepTimerTimeNumber, entity_category=ENTITY_CATEGORY_CONFIG),
        }
    ),
    cv.has_at_least_one_key(
        CONF_LIGHT_EFFECT_SPEED,
        CONF_FILTRATION_HOURS,
        CONF_FILTRATION_BLOCK_HOURS,
        CONF_SLEEP_TIMER_1_BEGIN_HOUR,
        CONF_SLEEP_TIMER_1_BEGIN_MINUTE,
        CONF_SLEEP_TIMER_1_END_HOUR,
        CONF_SLEEP_TIMER_1_END_MINUTE,
        CONF_SLEEP_TIMER_2_BEGIN_HOUR,
        CONF_SLEEP_TIMER_2_BEGIN_MINUTE,
        CONF_SLEEP_TIMER_2_END_HOUR,
        CONF_SLEEP_TIMER_2_END_MINUTE,
    ),
)

# (conf_key, timer_index, endpoint, field, min_value, max_value)
_SLEEP_TIMER_TIME_MAP = [
    (CONF_SLEEP_TIMER_1_BEGIN_HOUR, 1, SleepTimerEndpoint.kBegin, SleepTimerField.kHour, 0, 23),
    (CONF_SLEEP_TIMER_1_BEGIN_MINUTE, 1, SleepTimerEndpoint.kBegin, SleepTimerField.kMinute, 0, 59),
    (CONF_SLEEP_TIMER_1_END_HOUR, 1, SleepTimerEndpoint.kEnd, SleepTimerField.kHour, 0, 23),
    (CONF_SLEEP_TIMER_1_END_MINUTE, 1, SleepTimerEndpoint.kEnd, SleepTimerField.kMinute, 0, 59),
    (CONF_SLEEP_TIMER_2_BEGIN_HOUR, 2, SleepTimerEndpoint.kBegin, SleepTimerField.kHour, 0, 23),
    (CONF_SLEEP_TIMER_2_BEGIN_MINUTE, 2, SleepTimerEndpoint.kBegin, SleepTimerField.kMinute, 0, 59),
    (CONF_SLEEP_TIMER_2_END_HOUR, 2, SleepTimerEndpoint.kEnd, SleepTimerField.kHour, 0, 23),
    (CONF_SLEEP_TIMER_2_END_MINUTE, 2, SleepTimerEndpoint.kEnd, SleepTimerField.kMinute, 0, 59),
]


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
            var, config[CONF_FILTRATION_HOURS], min_value=1, max_value=24, step=1
        )
        cg.add(var.traits.set_mode(number.NUMBER_MODES["BOX"]))

    if CONF_FILTRATION_BLOCK_HOURS in config:
        var = cg.new_Pvariable(config[CONF_FILTRATION_BLOCK_HOURS][CONF_ID], parent)
        await cg.register_component(var, config[CONF_FILTRATION_BLOCK_HOURS])
        await number.register_number(
            var, config[CONF_FILTRATION_BLOCK_HOURS], min_value=1, max_value=24, step=1
        )
        cg.add(var.traits.set_mode(number.NUMBER_MODES["BOX"]))

    for conf_key, timer_index, endpoint, field, min_val, max_val in _SLEEP_TIMER_TIME_MAP:
        if conf_key not in config:
            continue
        var = cg.new_Pvariable(config[conf_key][CONF_ID], parent, timer_index, endpoint, field)
        await cg.register_component(var, config[conf_key])
        await number.register_number(var, config[conf_key], min_value=min_val, max_value=max_val, step=1)
        cg.add(var.traits.set_mode(number.NUMBER_MODES["BOX"]))
