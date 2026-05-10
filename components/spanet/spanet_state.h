#pragma once

#include <string>

namespace esphome::spanet {

struct ControllerIdentity {
  std::string software_version;
  std::string model;
  std::string serial_number_1;
  std::string serial_number_2;

  bool empty() const {
    return this->software_version.empty() && this->model.empty() && this->serial_number_1.empty() &&
           this->serial_number_2.empty();
  }
};

struct SpaNetState {
  ControllerIdentity controller;
};

}  // namespace esphome::spanet
