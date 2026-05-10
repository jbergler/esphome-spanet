import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor, uart
from esphome.const import CONF_ID

from . import SpaNetComponent

CONF_CONTROLLER = "controller"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(SpaNetComponent),
            cv.Required(CONF_CONTROLLER): text_sensor.text_sensor_schema(),
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    controller = await text_sensor.new_text_sensor(config[CONF_CONTROLLER])
    cg.add(var.set_controller_sensor(controller))
