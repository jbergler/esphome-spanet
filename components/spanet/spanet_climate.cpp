#include "spanet_climate.h"

#include <cmath>

#include "esphome/core/log.h"

namespace esphome::spanet {

static const char *const TAG = "spanet.climate";
static constexpr uint32_t COMMAND_TIMEOUT_MS = 1000;
static constexpr float SETPOINT_MIN_C = 5.0f;
static constexpr float SETPOINT_MAX_C = 41.0f;

void SpaNetClimate::setup() {
  this->mode = climate::CLIMATE_MODE_HEAT;

  this->parent_->add_on_state_callback([this](const State &state) { this->handle_state_update_(state); });
  this->handle_state_update_(this->parent_->get_state());
}

void SpaNetClimate::dump_config() { LOG_CLIMATE("", "SpaNET Climate", this); }

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
    if (!this->request_setpoint_temperature_(call.get_target_temperature().value())) {
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

bool SpaNetClimate::request_setpoint_temperature_(float target_c) {
  auto maybe_target_tenths = this->quantize_and_encode_setpoint_(target_c);
  if (!maybe_target_tenths.has_value()) {
    ESP_LOGW(TAG, "Rejected invalid/out-of-range setpoint %.2fC", target_c);
    return false;
  }

  this->parent_->enqueue_command_(Command{
      .kind = CommandKind::kSetpointWrite,
      .payload = "W40:" + std::to_string(maybe_target_tenths.value()),
      .expected_acks = {std::to_string(maybe_target_tenths.value())},
      .timeout_ms = COMMAND_TIMEOUT_MS,
      .triggers_rf_poll = true,
  });
  return true;
}

std::optional<int> SpaNetClimate::quantize_and_encode_setpoint_(float target_c) {
  if (std::isnan(target_c)) {
    return std::nullopt;
  }

  const float quantized_c = std::round(target_c * 5.0f) / 5.0f;
  if (quantized_c < SETPOINT_MIN_C || quantized_c > SETPOINT_MAX_C) {
    return std::nullopt;
  }

  return static_cast<int>(std::lround(quantized_c * 10.0f));
}

}  // namespace esphome::spanet
