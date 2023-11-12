#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <nlohmann/json.hpp>
#include <random>
#include <set>
#include <zmq.hpp>

#include "event/event/event.hpp"
#include "world/world/world.hpp"

using json = nlohmann::json;

const sf::Keyboard::Key key_list[] = {
    sf::Keyboard::Up,
    sf::Keyboard::Down,
    sf::Keyboard::Left,
    sf::Keyboard::Right,
};

void send_event(zmq::socket_t& s, const event& e) {
  std::string event_json = json(e).dump();
  s.send(zmq::message_t{event_json.begin(), event_json.end()},
         zmq::send_flags::none);
  zmq::message_t dummy_message;
  zmq::recv_result_t recv_result;
  recv_result = s.recv(dummy_message);
}

int main() {
  unsigned client_id = std::random_device{}();
  std::set<sf::Keyboard::Key> pressed_key;
  // setup sockets
  zmq::context_t context;
  zmq::socket_t event_socket(context, ZMQ_REQ);
  event_socket.connect("tcp://127.0.0.1:5555");
  zmq::socket_t world_socket(context, ZMQ_REQ);
  world_socket.connect("tcp://127.0.0.1:5556");
  // spawn character
  send_event(event_socket, {"character_spawn", {{"id", (double)client_id}}});
  // create window
  sf::VideoMode mode{800, 600};
  sf::RenderWindow window(mode, "Homework 4", sf::Style::Close);
  while (window.isOpen()) {
    // handle window close
    sf::Event e;
    while (window.pollEvent(e)) {
      if (e.type == sf::Event::Closed) {
        send_event(event_socket,
                   {"character_death", {{"id", (double)client_id}}});
        window.close();
      } else if (e.type == sf::Event::KeyPressed) {
        sf::Keyboard::Key key = e.key.code;
        if (pressed_key.count(key) == 0) {
          send_event(
              event_socket,
              {"key_press", {{"id", (double)client_id}, {"key", (double)key}}});
          pressed_key.insert(key);
        }
      } else if (e.type == sf::Event::KeyReleased) {
        sf::Keyboard::Key key = e.key.code;
        if (pressed_key.count(key) > 0) {
          send_event(event_socket,
                     {"key_release",
                      {{"id", (double)client_id}, {"key", (double)key}}});
          pressed_key.erase(key);
        }
      }
    }
    // request world data
    std::string data_req = json{{"id", client_id}}.dump();
    world_socket.send(zmq::message_t{data_req.begin(), data_req.end()},
                      zmq::send_flags::none);
    zmq::message_t world_data;
    zmq::recv_result_t recv_result = world_socket.recv(world_data);
    // std::cout << world_data.to_string() << std::endl;
    // draw world objects
    window.clear();
    world world_object = json::parse(world_data.to_string());
    if (world_object.replaying) {
      sf::Font f;
      f.loadFromFile("./NotoSans-Regular.ttf");
      sf::Text t{"Replaying...", f, 20};
      t.setFillColor(sf::Color::Red);
      window.draw(t);
    }
    for (const auto& [id, ch] : world_object.characters) {
      sf::RectangleShape rectangle{{30, 30}};
      rectangle.setPosition(ch.position[0], ch.position[1]);
      window.draw(rectangle);
    }
    window.display();
  }
}