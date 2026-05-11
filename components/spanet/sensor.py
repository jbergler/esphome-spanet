import esphome.codegen as cg
import esphome.config_validation as cv

from . import CONF_SPANET_ID, SpaNetComponent

DEPENDENCIES = ["spanet"]

# Placeholder sensor platform for compound component structure.
# Real numeric sensors can be added incrementally in later steps.
CONFIG_SCHEMA = cv.Schema({cv.GenerateID(CONF_SPANET_ID): cv.use_id(SpaNetComponent)})


async def to_code(config):
    # No-op for step 1.
    _ = await cg.get_variable(config[CONF_SPANET_ID])
