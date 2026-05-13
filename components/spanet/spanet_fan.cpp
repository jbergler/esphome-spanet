#include "spanet_fan.h"

#include <algorithm>
#include <cstring>

#include "esphome/core/log.h"

namespace esphome::spanet {

static const char *const TAG = "spanet.fan";
static const char *const AUTO_PRESET = "auto";
static constexpr uint32_t COMMAND_TIMEOUT_MS = 1000;

// For speed_type=2 pumps, controller command values 2/3 are inverted compared
// to observed readback raw modes in R5. Keep readback semantics in state/fan
// code and normalize only the outbound S22..S26 payload here.
static int encode_pump_command_mode(const PumpStatus &pump, int desired_raw_mode) {
  if (pump.speed_type == 2 && (desired_raw_mode == 2 || desired_raw_mode == 3)) {
    return desired_raw_mode == 2 ? 3 : 2;
  }
  return desired_raw_mode;
}

static size_t manual_mode_count_limit(const PumpStatus &pump) {
  return std::min(pump.manual_raw_mode_count, pump.manual_raw_modes.size());
}

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
  const size_t manual_mode_count = manual_mode_count_limit(pump);
  if (pump.installed && pump.capabilities_valid && pump.supports_speed) {
    traits.set_speed(true);
    traits.set_supported_speed_count(static_cast<int>(manual_mode_count));
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
  const size_t manual_mode_count = manual_mode_count_limit(pump);

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
    if (manual_mode_count == 0) {
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
      if (clamped_speed > static_cast<int>(manual_mode_count)) {
        clamped_speed = static_cast<int>(manual_mode_count);
      }
      next_raw_mode = pump.manual_raw_modes[static_cast<size_t>(clamped_speed - 1)];
    }
  } else if (call.get_state().has_value() && call.get_state().value()) {
    if (manual_mode_count > 0) {
      next_raw_mode = pump.manual_raw_modes[manual_mode_count - 1];
    } else if (pump.supports_auto) {
      next_raw_mode = 4;
    } else {
      ESP_LOGW(TAG, "Ignoring ON command for pump %u with no supported modes", this->pump_index_);
      return;
    }
  }

  if (!this->request_pump_mode_(next_raw_mode)) {
    ESP_LOGW(TAG, "Pump %u command rejected", this->pump_index_);
  }
}

int SpaNetPumpFan::resolve_manual_speed_(const PumpStatus &pump) const {
  if (!pump.current_raw_mode.has_value()) {
    return 0;
  }

  // Reverse lookup of the same RG-declared manual mode order used for command
  // writes. This keeps published speed values aligned with the UI mapping.
  const int raw_mode = pump.current_raw_mode.value();
  const size_t manual_mode_count = manual_mode_count_limit(pump);
  for (size_t i = 0; i < manual_mode_count; i++) {
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
    const size_t manual_mode_count = manual_mode_count_limit(pump);
    if (next_speed == 0 && manual_mode_count > 0) {
      next_speed = static_cast<int>(manual_mode_count);
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

bool SpaNetPumpFan::request_pump_mode_(int raw_mode) {
  if (raw_mode < 0 || raw_mode > 4) {
    ESP_LOGW(TAG, "Rejected invalid pump mode %d for pump %u", raw_mode, this->pump_index_);
    return false;
  }

  const auto &pump = this->parent_->get_state().pumps[this->pump_index_ - 1];
  if (!pump.installed || !pump.capabilities_valid) {
    ESP_LOGW(TAG, "Rejected pump%u command while pump is unavailable", this->pump_index_);
    return false;
  }

  if (!pump.supports_raw_mode[static_cast<size_t>(raw_mode)]) {
    ESP_LOGW(TAG, "Rejected unsupported pump%u mode %d", this->pump_index_, raw_mode);
    return false;
  }

  const int command_mode = encode_pump_command_mode(pump, raw_mode);
  if (command_mode != raw_mode) {
    ESP_LOGV(TAG, "Pump%u translating desired mode %d to wire mode %d (speed_type=%d)", this->pump_index_, raw_mode,
             command_mode, pump.speed_type);
  }

  const int command_family = 21 + static_cast<int>(this->pump_index_);
  const std::string command_prefix = "S" + std::to_string(command_family);
  this->parent_->enqueue_command_(QueuedCommand{
      .kind = CommandKind::kPumpWrite,
      .payload = command_prefix + ":" + std::to_string(command_mode),
      .expected_ack = command_prefix + "-OK",
      .timeout_ms = COMMAND_TIMEOUT_MS,
  });
  return true;
}

}  // namespace esphome::spanet
