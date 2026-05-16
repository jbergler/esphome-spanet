import esphome.codegen as cg
import esphome.config_validation as cv
import esphome.automation as automation
from esphome.components import time as time_, uart
from esphome.const import CONF_ID, CONF_TIME_ID

CODEOWNERS = ["@jbergler"]
DEPENDENCIES = ["uart"]
AUTO_LOAD = ["text_sensor", "sensor", "climate", "fan", "light", "select", "number", "time", "binary_sensor"]

CONF_SPANET_ID = "spanet_id"
CONF_AUTO_SYNC_INTERVAL = "auto_sync_interval"
CONF_UNIX_TIMESTAMP = "unix_timestamp"

spanet_ns = cg.esphome_ns.namespace("spanet")
SpaNetComponent = spanet_ns.class_("SpaNetComponent", cg.PollingComponent)
SpaNetSetCurrentTimeAction = spanet_ns.class_("SpaNetSetCurrentTimeAction", automation.Action)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(SpaNetComponent),
            cv.Optional(CONF_TIME_ID): cv.use_id(time_.RealTimeClock),
            cv.Optional(CONF_AUTO_SYNC_INTERVAL, default="24h"): cv.positive_time_period_milliseconds,
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)

SPANET_SET_CURRENT_TIME_ACTION_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_SPANET_ID): cv.use_id(SpaNetComponent),
        cv.Optional(CONF_UNIX_TIMESTAMP): cv.templatable(cv.positive_int),
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if CONF_TIME_ID in config:
        time_source = await cg.get_variable(config[CONF_TIME_ID])
        cg.add(var.set_time_source(time_source))
        cg.add(var.set_auto_sync_time(True))
        cg.add(var.set_auto_sync_interval_ms(config[CONF_AUTO_SYNC_INTERVAL].total_milliseconds))


@automation.register_action(
    "spanet.set_current_time",
    SpaNetSetCurrentTimeAction,
    SPANET_SET_CURRENT_TIME_ACTION_SCHEMA,
)
async def spanet_set_current_time_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_SPANET_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)

    if CONF_UNIX_TIMESTAMP in config:
        unix_timestamp = await cg.templatable(config[CONF_UNIX_TIMESTAMP], args, cg.uint32)
        cg.add(var.set_unix_timestamp(unix_timestamp))

    return var
