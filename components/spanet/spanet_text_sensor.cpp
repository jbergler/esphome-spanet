#include "spanet_text_sensor.h"

#include "esphome/core/log.h"

namespace esphome::spanet {

static const char *const TAG = "spanet.text_sensor";

void SpaNetTextSensor::setup() {
  this->parent_->add_on_state_callback([this](const State &state) { this->handle_state_update_(state); });
  this->handle_state_update_(this->parent_->get_state());
}

void SpaNetTextSensor::dump_config() { LOG_TEXT_SENSOR("", "SpaNET Text Sensor", this); }

void SpaNetTextSensor::handle_state_update_(const State &state) {
  std::string value;
  switch (this->kind_) {
    case kControllerModel:
      value = state.controller_status.model;
      break;
    case kControllerSerial:
      value = state.controller_status.serial_number;
      break;
    case kControllerFwVersion:
      value = state.controller_status.software_version;
      break;
    case kCurrentTime:
      if (!state.controller_status.current_time.has_value()) {
        return;
      }
      value = SpaNetTimeSync::format_time_text(state.controller_status.current_time.value());
      break;
  }
  if (!value.empty() && this->get_raw_state() != value) {
    this->publish_state(value);
  }
}

}  // namespace esphome::spanet
