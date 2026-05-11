#pragma once

#include <ctime>
#include <optional>
#include <string>
#include <variant>

#include "spanet_parser.h"

namespace esphome::spanet {

struct ControllerStatus {
  std::string software_version;
  std::string model;
  std::string serial_number_1;
  std::string serial_number_2;
  std::optional<time_t> current_time;
};

struct State {
  ControllerStatus controller_status;
};

struct Registers {
  std::optional<RegisterR2> r2;
  std::optional<RegisterR3> r3;
  std::optional<RegisterR4> r4;
  std::optional<RegisterR5> r5;
  std::optional<RegisterR6> r6;
  std::optional<RegisterR7> r7;
  std::optional<RegisterR9> r9;
  std::optional<RegisterRA> ra;
  std::optional<RegisterRB> rb;
  std::optional<RegisterRC> rc;
  std::optional<RegisterRE> re;
  std::optional<RegisterRG> rg;

  bool update(const AnyRegisterLine &line);
};

struct StoreRegisterVisitor {
  Registers* target;

  bool operator()(const RegisterR2& reg) { target->r2 = reg; return true; }
  bool operator()(const RegisterR3& reg) { target->r3 = reg; return true; }
  bool operator()(const RegisterR4& reg) { target->r4 = reg; return true; }
  bool operator()(const RegisterR5& reg) { target->r5 = reg; return true; }
  bool operator()(const RegisterR6& reg) { target->r6 = reg; return true; }
  bool operator()(const RegisterR7& reg) { target->r7 = reg; return true; }
  bool operator()(const RegisterR9& reg) { target->r9 = reg; return true; }
  bool operator()(const RegisterRA& reg) { target->ra = reg; return true; }
  bool operator()(const RegisterRB& reg) { target->rb = reg; return true; }
  bool operator()(const RegisterRC& reg) { target->rc = reg; return true; }
  bool operator()(const RegisterRE& reg) { target->re = reg; return true; }
  bool operator()(const RegisterRG& reg) { target->rg = reg; return true; }
  bool operator()(const UnknownRegisterLine&) { return true; }
};

class RegisterStore {
 public:
  RegisterStore();

  bool update(const std::string &line);

  const State &get_state() const;

  const Registers &get_registers() const;

 private:
  State state;
  Registers registers_ = {};

  void update_controller_status();
};

}  // namespace esphome::spanet
