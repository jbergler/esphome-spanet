#include "spanet_climate.h"

#include <cmath>

#include "esphome/core/log.h"

namespace esphome::spanet {

static const char *const TAG = "spanet.climate";

void SpaNetClimate::setup() {
  this->mode = climate::CLIMATE_MODE_HEAT;

  this->parent_->add_on_state_callback([this](const State &state) { this->handle_state_update_(state); });
  this->handle_state_update_(this->parent_->get_state());
}

void SpaNetClimate::dump_config() {
  LOG_CLIMATE("", "SpaNET Climate", this);
}

climate::ClimateTraits SpaNetClimate::traits() {
  climate::ClimateTraits traits;
  traits.set_supported_modes({climate::CLIMATE_MODE_OFF, climate::CLIMATE_MODE_HEAT});
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_ACTION);
  traits.set_visual_min_temperature(5.0f);
  traits.set_visual_max_temperature(41.0f);
  traits.set_visual_temperature_step(0.2f);
  return traits;
}

void SpaNetClimate::control(const climate::ClimateCall &call) {
  if (call.get_mode().has_value()) {
    ESP_LOGD(TAG, "Ignoring climate mode write in MVP");
  }

  if (call.get_target_temperature().has_value()) {
    if (!this->parent_->request_setpoint_temperature(call.get_target_temperature().value())) {
      ESP_LOGW(TAG, "Rejected setpoint request from climate control call");
    }
  }
}

void SpaNetClimate::handle_state_update_(const State &state) {
  bool changed = false;

  if (state.temperatures.water_c.has_value()) {
    const float next = state.temperatures.water_c.value();
    if (std::isnan(this->current_temperature) || this->current_temperature != next) {
      this->current_temperature = next;
      changed = true;
    }
  }

  if (state.temperatures.setpoint_c.has_value()) {
    const float next = state.temperatures.setpoint_c.value();
    if (std::isnan(this->target_temperature) || this->target_temperature != next) {
      this->target_temperature = next;
      changed = true;
    }
  }

  if (state.climate.heating_active.has_value()) {
    climate::ClimateAction next_action =
        state.climate.heating_active.value() ? climate::CLIMATE_ACTION_HEATING : climate::CLIMATE_ACTION_IDLE;
    if (this->action != next_action) {
      this->action = next_action;
      changed = true;
    }
  }

  if (this->mode != climate::CLIMATE_MODE_HEAT) {
    this->mode = climate::CLIMATE_MODE_HEAT;
    changed = true;
  }

  if (changed) {
    this->publish_state();
  }
}

}  // namespace esphome::spanet
