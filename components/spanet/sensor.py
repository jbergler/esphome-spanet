import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

from . import SpaNetComponent

# Placeholder sensor platform for compound component structure.
# Real numeric sensors can be added incrementally in later steps.
CONFIG_SCHEMA = cv.Schema({cv.GenerateID(): cv.use_id(SpaNetComponent)})


async def to_code(config):
    # No-op for step 1.
    _ = await cg.get_variable(config[CONF_ID])
