import esphome.codegen as cg

CODEOWNERS = ["@jbergler"]

spanet_ns = cg.esphome_ns.namespace("spanet")
SpaNetComponent = spanet_ns.class_("SpaNetComponent", cg.PollingComponent)
