#include "spanet_filt_block_hrs_number.h"

#include <cmath>

#include "esphome/core/log.h"

namespace esphome::spanet {

static const char *const TAG = "spanet.filt_block_hrs";
static constexpr uint32_t COMMAND_TIMEOUT_MS = 1000;

// Valid filtration block duration values accepted by the spa controller.
static constexpr std::array<int, 8> VALID_BLOCK_HRS{{1, 2, 3, 4, 6, 8, 12, 24}};

static int snap_to_valid_block_hrs(int requested) {
  int best = VALID_BLOCK_HRS[0];
  int best_diff = std::abs(requested - best);
  for (size_t i = 1; i < VALID_BLOCK_HRS.size(); i++) {
    int diff = std::abs(requested - VALID_BLOCK_HRS[i]);
    if (diff < best_diff) {
      best_diff = diff;
      best = VALID_BLOCK_HRS[i];
    }
  }
  return best;
}

void SpaNetFiltBlockHrsNumber::setup() {
  this->parent_->add_on_state_callback([this](const State &state) { this->handle_state_update_(state); });
  this->handle_state_update_(this->parent_->get_state());
}

void SpaNetFiltBlockHrsNumber::dump_config() { LOG_NUMBER("", "SpaNET Filtration Block Duration", this); }

void SpaNetFiltBlockHrsNumber::control(float value) {
  int hrs = snap_to_valid_block_hrs(static_cast<int>(std::lround(value)));

  if (!this->request_filt_block_hrs_(hrs)) {
    ESP_LOGW(TAG, "Filtration block hours request rejected");
    return;
  }
}

void SpaNetFiltBlockHrsNumber::handle_state_update_(const State &state) {
  if (!state.filtration.block_hrs.has_value()) {
    return;
  }

  const int hrs = state.filtration.block_hrs.value();

  if (this->has_published_state_ && this->last_block_hrs_ == hrs) {
    return;
  }

  this->publish_state(static_cast<float>(hrs));
  this->has_published_state_ = true;
  this->last_block_hrs_ = hrs;
}

bool SpaNetFiltBlockHrsNumber::request_filt_block_hrs_(int hrs) {
  bool valid = false;
  for (int v : VALID_BLOCK_HRS) {
    if (hrs == v) {
      valid = true;
      break;
    }
  }
  if (!valid) {
    ESP_LOGW(TAG, "Rejected invalid filtration block hours %d", hrs);
    return false;
  }
  const std::string hrs_str = std::to_string(hrs);
  this->parent_->enqueue_command_(Command{
      .kind = CommandKind::kFiltBlockHrsWrite,
      .payload = "W90:" + hrs_str,
      .expected_acks = {hrs_str},
      .timeout_ms = COMMAND_TIMEOUT_MS,
      .triggers_rf_poll = true,
  });
  return true;
}

}  // namespace esphome::spanet
