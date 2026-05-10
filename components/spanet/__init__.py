import esphome.codegen as cg

CODEOWNERS = ["@jbergler"]
DEPENDENCIES = ["uart"]
AUTO_LOAD = ["text_sensor", "sensor"]

spanet_ns = cg.esphome_ns.namespace("spanet")
SpaNetComponent = spanet_ns.class_("SpaNetComponent", cg.PollingComponent)
