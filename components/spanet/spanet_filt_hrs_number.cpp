#include "spanet_filt_hrs_number.h"

#include <cmath>

#include "esphome/core/log.h"

namespace esphome::spanet {

static const char *const TAG = "spanet.filt_hrs";
static constexpr uint32_t COMMAND_TIMEOUT_MS = 1000;

void SpaNetFiltHrsNumber::setup() {
  this->parent_->add_on_state_callback([this](const State &state) { this->handle_state_update_(state); });
  this->handle_state_update_(this->parent_->get_state());
}

void SpaNetFiltHrsNumber::dump_config() { LOG_NUMBER("", "SpaNET Filtration Hours", this); }

void SpaNetFiltHrsNumber::control(float value) {
  int hrs = static_cast<int>(std::lround(value));
  if (hrs < 1) {
    hrs = 1;
  } else if (hrs > 24) {
    hrs = 24;
  }

  if (!this->request_filt_hrs_(hrs)) {
    ESP_LOGW(TAG, "Filtration hours request rejected");
    return;
  }
}

void SpaNetFiltHrsNumber::handle_state_update_(const State &state) {
  if (!state.filtration.set_hrs.has_value()) {
    return;
  }

  const int hrs = state.filtration.set_hrs.value();

  if (this->has_published_state_ && this->last_hrs_ == hrs) {
    return;
  }

  this->publish_state(static_cast<float>(hrs));
  this->has_published_state_ = true;
  this->last_hrs_ = hrs;
}

bool SpaNetFiltHrsNumber::request_filt_hrs_(int hrs) {
  if (hrs < 1 || hrs > 24) {
    ESP_LOGW(TAG, "Rejected invalid filtration hours %d (range 1-24)", hrs);
    return false;
  }
  const std::string hrs_str = std::to_string(hrs);
  this->parent_->enqueue_command_(Command{
      .kind = CommandKind::kFiltHrsWrite,
      .payload = "W60:" + hrs_str,
      .expected_acks = {hrs_str},
      .timeout_ms = COMMAND_TIMEOUT_MS,
      .triggers_rf_poll = true,
  });
  return true;
}

}  // namespace esphome::spanet
