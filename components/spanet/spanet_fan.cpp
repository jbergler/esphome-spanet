#include "spanet_fan.h"

#include <cstring>

#include "esphome/core/log.h"

namespace esphome::spanet {

static const char *const TAG = "spanet.fan";
static const char *const AUTO_PRESET = "auto";

void SpaNetPumpFan::setup() {
  this->parent_->add_on_state_callback([this](const State &state) { this->handle_state_update_(state); });
  this->handle_state_update_(this->parent_->get_state());
}

void SpaNetPumpFan::dump_config() {
  LOG_FAN("", "SpaNET Pump Fan", this);
  ESP_LOGCONFIG(TAG, "  Pump Index: %u", this->pump_index_);
}

fan::FanTraits SpaNetPumpFan::get_traits() {
  fan::FanTraits traits;
  traits.set_oscillation(false);
  traits.set_direction(false);

  const auto &pump = this->parent_->get_state().pumps[this->pump_index_ - 1];
  if (pump.installed && pump.capabilities_valid && pump.supports_speed) {
    traits.set_speed(true);
    traits.set_supported_speed_count(static_cast<int>(pump.manual_raw_mode_count));
  } else {
    traits.set_speed(false);
    traits.set_supported_speed_count(1);
  }

  this->wire_preset_modes_(traits);
  return traits;
}

void SpaNetPumpFan::control(const fan::FanCall &call) {
  const auto &pump = this->parent_->get_state().pumps[this->pump_index_ - 1];
  if (!pump.installed || !pump.capabilities_valid) {
    ESP_LOGW(TAG, "Ignoring command for unavailable pump %u", this->pump_index_);
    return;
  }

  int next_raw_mode = pump.current_raw_mode.value_or(0);

  if (call.get_state().has_value() && !call.get_state().value()) {
    next_raw_mode = 0;
  } else if (call.has_preset_mode()) {
    if (std::strcmp(call.get_preset_mode(), AUTO_PRESET) != 0 || !pump.supports_auto) {
      ESP_LOGW(TAG, "Ignoring unsupported preset for pump %u", this->pump_index_);
      return;
    }
    next_raw_mode = 4;
  } else if (call.get_speed().has_value()) {
    const int requested_speed = call.get_speed().value();
    if (pump.manual_raw_mode_count == 0) {
      ESP_LOGW(TAG, "Ignoring speed command without manual speeds for pump %u", this->pump_index_);
      return;
    }

    // Speed n maps to manual_raw_modes[n-1]. The store preserves RG-declared
    // mode order, which lets us honor controller-specific low/high semantics
    // even when raw numeric values are not ascending by user-facing speed.
    if (!pump.supports_speed) {
      next_raw_mode = pump.manual_raw_modes[0];
    } else {
      int clamped_speed = requested_speed;
      if (clamped_speed < 1) {
        clamped_speed = 1;
      }
      if (clamped_speed > static_cast<int>(pump.manual_raw_mode_count)) {
        clamped_speed = static_cast<int>(pump.manual_raw_mode_count);
      }
      next_raw_mode = pump.manual_raw_modes[static_cast<size_t>(clamped_speed - 1)];
    }
  } else if (call.get_state().has_value() && call.get_state().value()) {
    if (pump.manual_raw_mode_count > 0) {
      next_raw_mode = pump.manual_raw_modes[static_cast<size_t>(pump.manual_raw_mode_count - 1)];
    } else if (pump.supports_auto) {
      next_raw_mode = 4;
    } else {
      ESP_LOGW(TAG, "Ignoring ON command for pump %u with no supported modes", this->pump_index_);
      return;
    }
  }

  if (!this->parent_->request_pump_mode(this->pump_index_, next_raw_mode)) {
    ESP_LOGW(TAG, "Pump %u command rejected by hub", this->pump_index_);
  }
}

int SpaNetPumpFan::resolve_manual_speed_(const PumpStatus &pump) const {
  if (!pump.current_raw_mode.has_value()) {
    return 0;
  }

  // Reverse lookup of the same RG-declared manual mode order used for command
  // writes. This keeps published speed values aligned with the UI mapping.
  const int raw_mode = pump.current_raw_mode.value();
  for (size_t i = 0; i < pump.manual_raw_mode_count; i++) {
    if (pump.manual_raw_modes[i] == raw_mode) {
      return static_cast<int>(i + 1);
    }
  }

  return 0;
}

void SpaNetPumpFan::sync_supported_presets_(const PumpStatus &pump) {
  if (pump.installed && pump.capabilities_valid && pump.supports_auto) {
    this->set_supported_preset_modes({AUTO_PRESET});
  } else {
    this->set_supported_preset_modes({});
  }
}

void SpaNetPumpFan::handle_state_update_(const State &state) {
  const auto &pump = state.pumps[this->pump_index_ - 1];
  this->sync_supported_presets_(pump);

  if (!pump.installed || !pump.capabilities_valid) {
    this->has_published_state_ = false;
    return;
  }

  bool changed = false;
  const bool next_state = pump.is_on;
  if (this->state != next_state) {
    this->state = next_state;
    changed = true;
  }

  int next_speed = 0;
  if (next_state) {
    next_speed = this->resolve_manual_speed_(pump);
    if (next_speed == 0 && pump.manual_raw_mode_count > 0) {
      next_speed = static_cast<int>(pump.manual_raw_mode_count);
    }
  }

  if (this->speed != next_speed) {
    this->speed = next_speed;
    changed = true;
  }

  if (pump.auto_mode_active && pump.supports_auto) {
    if (this->set_preset_mode_(AUTO_PRESET)) {
      changed = true;
    }
  } else if (this->has_preset_mode()) {
    this->clear_preset_mode_();
    changed = true;
  }

  if (!this->has_published_state_ || changed) {
    this->publish_state();
    this->has_published_state_ = true;
  }
}

}  // namespace esphome::spanet
