import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID

CODEOWNERS = ["@jbergler"]
DEPENDENCIES = ["uart"]
AUTO_LOAD = ["text_sensor", "sensor", "climate"]

CONF_SPANET_ID = "spanet_id"

spanet_ns = cg.esphome_ns.namespace("spanet")
SpaNetComponent = spanet_ns.class_("SpaNetComponent", cg.PollingComponent)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(SpaNetComponent),
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
