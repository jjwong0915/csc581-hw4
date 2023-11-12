#include "world/world/world.hpp"

#include <iostream>

void to_json(json& j, const world& w) {
  j = {
      {"time", w.time},
      {"characters", w.characters},
      {"replaying", w.replaying},
  };
}

void from_json(const json& j, world& w) {
  j.at("time").get_to(w.time);
  j.at("characters").get_to(w.characters);
  j.at("replaying").get_to(w.replaying);
}
