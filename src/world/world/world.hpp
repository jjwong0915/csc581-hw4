#ifndef WORLD_HPP
#define WORLD_HPP

#include <map>
#include <nlohmann/json.hpp>

#include "event/timestamp/timestamp.hpp"
#include "world/character/character.hpp"

using json = nlohmann::json;

struct world {
  timestamp time;
  std::map<unsigned, character> characters;
  bool replaying = false;
};

void to_json(json& j, const world& w);
void from_json(const json& j, world& w);

#endif