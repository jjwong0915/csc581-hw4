#include <SFML/Window.hpp>
#include <cmath>
#include <iostream>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include <random>
#include <thread>
#include <variant>
#include <zmq.hpp>

#include "event/event/event.hpp"
#include "event/event_manager/event_manager.hpp"
#include "event/timeline/timeline.hpp"
#include "world/world/world.hpp"

using json = nlohmann::json;

struct recording {
  world record_world;
  timestamp record_end;
  timestamp replay_end;
};

// global states
std::mutex global_mtx;
event_manager manager;
world common_world;
std::map<unsigned, recording> client_recording;
// random number generator
std::random_device rd;
std::default_random_engine gen(rd());

void handle_character_spawn(world& w, const event& e) {
  unsigned id = (unsigned)std::get<double>(e.param.at("id"));
  w.characters[id] = {
      id,
      {std::uniform_real_distribution{0.f, 770.f}(gen),
       std::uniform_real_distribution{0.f, 570.f}(gen)},
  };
}

void handle_character_death(world& w, const event& e) {
  unsigned id = (unsigned)std::get<double>(e.param.at("id"));
  w.characters.erase(id);
}

void handle_character_collision(world& w, const event& e) {
  unsigned character1 = (unsigned)std::get<double>(e.param.at("character1"));
  unsigned character2 = (unsigned)std::get<double>(e.param.at("character2"));
  if (w.characters.count(character1) > 0 &&
      w.characters.count(character2) > 0) {
    w.characters[character1].collided = true;
    w.characters[character2].collided = true;
    if (!w.replaying) {
      manager.raise_event({"character_death", {{"id", (double)character1}}});
      manager.raise_event({"character_death", {{"id", (double)character2}}});
      manager.raise_event({"character_spawn", {{"id", (double)character1}}});
      manager.raise_event({"character_spawn", {{"id", (double)character2}}});
    }
  }
}

void handle_character_movement(world& w, const event& e) {
  unsigned id = (unsigned)std::get<double>(e.param.at("id"));
  float new_x = (float)std::get<double>(e.param.at("new_x"));
  float new_y = (float)std::get<double>(e.param.at("new_y"));
  if (w.characters.count(id) > 0 &&
      !(!w.replaying && client_recording.count(id) > 0 &&
        client_recording[id].replay_end != timestamp{})) {
    w.characters[id].position = {new_x, new_y};
  }
}

void handle_key_press(world& w, const event& e) {
  // std::cout << "key pressed" << std::endl;
  unsigned id = (unsigned)std::get<double>(e.param.at("id"));
  if (w.characters.count(id) > 0) {
    sf::Keyboard::Key key =
        (sf::Keyboard::Key)std::get<double>(e.param.at("key"));
    if (key == sf::Keyboard::Up) {
      w.characters[id].moving_up = true;
    } else if (key == sf::Keyboard::Down) {
      w.characters[id].moving_down = true;
    } else if (key == sf::Keyboard::Left) {
      w.characters[id].moving_left = true;
    } else if (key == sf::Keyboard::Right) {
      w.characters[id].moving_right = true;
    } else if (key == sf::Keyboard::R) {
      if (!w.replaying) {
        if (client_recording.count(id) == 0) {
          manager.raise_event({"starting_recording", {{"id", (double)id}}});
        } else if (client_recording[id].record_end == timestamp{}) {
          manager.raise_event({"stopping_recording", {{"id", (double)id}}});
        }
      }
    }
  }
}

void handle_key_release(world& w, const event& e) {
  // std::cout << "key release" << std::endl;
  unsigned id = (unsigned)std::get<double>(e.param.at("id"));
  if (w.characters.count(id) > 0) {
    sf::Keyboard::Key key =
        (sf::Keyboard::Key)std::get<double>(e.param.at("key"));
    if (key == sf::Keyboard::Up) {
      w.characters[id].moving_up = false;
    } else if (key == sf::Keyboard::Down) {
      w.characters[id].moving_down = false;
    } else if (key == sf::Keyboard::Left) {
      w.characters[id].moving_left = false;
    } else if (key == sf::Keyboard::Right) {
      w.characters[id].moving_right = false;
    }
  }
}

void handle_starting_recording(world& w, const event& e) {
  // std::cout << "start recoding" << std::endl;
  unsigned id = (unsigned)std::get<double>(e.param.at("id"));
  if (client_recording.count(id) > 0) {
    return;  // already started recording
  }
  client_recording.insert({id, {w}});
}

void handle_stopping_recording(world& w, const event& e) {
  // std::cout << "stop recoding" << std::endl;
  unsigned id = (unsigned)std::get<double>(e.param.at("id"));
  if (client_recording.count(id) == 0) {
    return;  // haven't started recording
  }
  client_recording[id].record_world.replaying = true;
  client_recording[id].record_end = e.time;
  client_recording[id].replay_end =
      e.time + (e.time - client_recording[id].record_world.time);
}

void receive_event() {
  // setup socket
  zmq::context_t context;
  zmq::socket_t event_socket{context, ZMQ_REP};
  event_socket.bind("tcp://127.0.0.1:5555");
  // handle request
  while (true) {
    // std::cout << "handle event begin" << std::endl;
    zmq::message_t event_req;
    zmq::recv_result_t recv_result = event_socket.recv(event_req);
    {
      std::lock_guard lock{global_mtx};
      manager.raise_event(json::parse(event_req.to_string()));
    }
    event_socket.send(zmq::message_t{}, zmq::send_flags::none);
    // std::cout << "handle event end" << std::endl;
  }
}

void send_world() {
  // setup socket
  zmq::context_t context;
  zmq::socket_t world_socket{context, ZMQ_REP};
  world_socket.bind("tcp://127.0.0.1:5556");
  // handle request
  while (true) {
    // std::cout << "handle world begin" << std::endl;
    zmq::message_t world_req;
    zmq::recv_result_t recv_result = world_socket.recv(world_req);
    unsigned client_id = json::parse(world_req.to_string()).at("id");
    // send world
    std::string world_json;
    {
      std::lock_guard lock{global_mtx};
      if (client_recording.count(client_id) > 0 &&
          client_recording[client_id].record_end != timestamp{}) {
        recording& r = client_recording[client_id];
        timestamp t = timeline::get_time();
        if (t <= r.replay_end) {
          manager.apply_events(r.record_world,
                               r.record_end - (r.replay_end - t));
          world_json = json(r.record_world).dump();
        } else {
          client_recording.erase(client_id);
          world_json = json(common_world).dump();
        }
      } else {
        world_json = json(common_world).dump();
      }
    }
    world_socket.send(zmq::message_t{world_json.begin(), world_json.end()},
                      zmq::send_flags::none);
    // std::cout << "handle world end" << std::endl;
  }
}

int main() {
  // register event handlers
  manager.register_handler("character_spawn", handle_character_spawn);
  manager.register_handler("character_death", handle_character_death);
  manager.register_handler("character_collision", handle_character_collision);
  manager.register_handler("character_movement", handle_character_movement);
  manager.register_handler("key_press", handle_key_press);
  manager.register_handler("key_release", handle_key_release);
  manager.register_handler("starting_recording", handle_starting_recording);
  manager.register_handler("stopping_recording", handle_stopping_recording);
  // start receiver and sender
  std::thread event_receiver{receive_event};
  std::thread world_sender{send_world};
  // update world
  timestamp prev_time = timeline::get_time();
  while (true) {
    {
      std::lock_guard lock{global_mtx};
      // raise movement events
      timestamp present_time = timeline::get_time();
      float diff_time = 0.000000001f * (present_time - prev_time).count();
      for (auto& [id, ch] : common_world.characters) {
        if (ch.collided) continue;
        //
        auto v = ch.velocity();
        manager.raise_event({"character_movement",
                             {{"id", (double)id},
                              {"new_x", ch.position[0] + v[0] * diff_time},
                              {"new_y", ch.position[1] + v[1] * diff_time}}});
      }
      // raise collision events
      for (auto& [id1, ch1] : common_world.characters) {
        for (auto& [id2, ch2] : common_world.characters) {
          if (id1 < id2 && !ch1.collided && !ch2.collided &&
              std::abs(ch1.position[0] - ch2.position[0]) < 30 &&
              std::abs(ch1.position[1] - ch2.position[1]) < 30) {
            manager.raise_event(
                {"character_collision",
                 {{"character1", (double)id1}, {"character2", (double)id2}}});
          }
        }
      }
      prev_time = present_time;
      // apply events
      manager.apply_events(common_world, timeline::get_time());
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}