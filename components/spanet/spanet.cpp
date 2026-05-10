#include "spanet.h"
#include "esphome/core/log.h"

namespace esphome::spanet {

static const char *const TAG = "spanet";

void SpaNetComponent::setup() {
  ESP_LOGI(TAG, "Setting up dummy SpaNET component");
}

void SpaNetComponent::update() {
  // Dummy hardcoded model value from component runtime logic.
  // In later steps this will come from parsed controller data.
  if (this->controller_sensor_ != nullptr) {
    this->controller_sensor_->publish_state("SVM1");
  }
}

void SpaNetComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "SpaNET dummy component");
  LOG_UPDATE_INTERVAL(this);
}

}  // namespace esphome::spanet
