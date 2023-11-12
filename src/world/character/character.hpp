#ifndef CHARACTER_HPP
#define CHARACTER_HPP

#include <array>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct character {
  unsigned id;
  std::array<float, 2> position;
  bool moving_up = false;
  bool moving_down = false;
  bool moving_left = false;
  bool moving_right = false;
  bool collided = false;
  //
  std::array<float, 2> velocity();
};

void to_json(json& j, const character& c);
void from_json(const json& j, character& c);

#endif