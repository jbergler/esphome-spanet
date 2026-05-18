import esphome.codegen as cg
import esphome.config_validation as cv
import esphome.automation as automation
from esphome.components import time as time_, uart, output, light
from esphome.const import CONF_ID, CONF_TIME_ID

CODEOWNERS = ["@jbergler"]
DEPENDENCIES = ["uart"]
AUTO_LOAD = ["text_sensor", "sensor", "climate", "fan", "light", "select", "number", "time", "binary_sensor"]

CONF_SPANET_ID = "spanet_id"
CONF_AUTO_SYNC_INTERVAL = "auto_sync_interval"
CONF_UNIX_TIMESTAMP = "unix_timestamp"
CONF_STATUS = "status"
CONF_LED = "led"
CONF_LEDS = "leds"
CONF_RGB_LIGHT = "rgb_light"

spanet_ns = cg.esphome_ns.namespace("spanet")
SpaNetComponent = spanet_ns.class_("SpaNetComponent", cg.PollingComponent)
SpaNetSetCurrentTimeAction = spanet_ns.class_("SpaNetSetCurrentTimeAction", automation.Action)
SpaNetStatusIndicator = spanet_ns.class_("SpaNetStatusIndicator", cg.Component)

STATUS_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(SpaNetStatusIndicator),
            cv.Exclusive(CONF_LED, "mode"): cv.use_id(output.BinaryOutput),
            cv.Exclusive(CONF_LEDS, "mode"): cv.ensure_list(cv.use_id(output.BinaryOutput)),
            cv.Exclusive(CONF_RGB_LIGHT, "mode"): cv.use_id(light.LightState),
        }
    ),
    cv.has_at_least_one_key(CONF_LED, CONF_LEDS, CONF_RGB_LIGHT),
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(SpaNetComponent),
            cv.Optional(CONF_TIME_ID): cv.use_id(time_.RealTimeClock),
            cv.Optional(CONF_AUTO_SYNC_INTERVAL, default="24h"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_STATUS): STATUS_SCHEMA,
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

    if CONF_STATUS in config:
        status_cfg = config[CONF_STATUS]
        indicator = cg.new_Pvariable(status_cfg[CONF_ID], var)
        await cg.register_component(indicator, status_cfg)

        if CONF_LED in status_cfg:
            led = await cg.get_variable(status_cfg[CONF_LED])
            cg.add(indicator.set_led(led))
        elif CONF_LEDS in status_cfg:
            for led_id in status_cfg[CONF_LEDS]:
                led = await cg.get_variable(led_id)
                cg.add(indicator.add_led(led))
        elif CONF_RGB_LIGHT in status_cfg:
            light_var = await cg.get_variable(status_cfg[CONF_RGB_LIGHT])
            cg.add(indicator.set_rgb_light(light_var))


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
