#include "world/character/character.hpp"

#include <iostream>

std::array<float, 2> character::velocity() {
  float x = 0.0, y = 0.0;
  if (moving_up) {
    y -= 100.0;
  }
  if (moving_down) {
    y += 100.0;
  }
  if (moving_left) {
    x -= 100.0;
  }
  if (moving_right) {
    x += 100.0;
  }
  return {x, y};
}

void to_json(json& j, const character& c) {
  j = {
      {"id", c.id},
      {"position", c.position},
      {"moving_up", c.moving_up},
      {"moving_down", c.moving_down},
      {"moving_left", c.moving_left},
      {"moving_right", c.moving_right},
  };
}

void from_json(const json& j, character& c) {
  j.at("id").get_to(c.id);
  j.at("position").get_to(c.position);
  j.at("moving_up").get_to(c.moving_up);
  j.at("moving_down").get_to(c.moving_down);
  j.at("moving_left").get_to(c.moving_left);
  j.at("moving_right").get_to(c.moving_right);
}
