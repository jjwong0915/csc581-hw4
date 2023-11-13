CSC 581 Homework 4
====
The main task of this homework is to design an event management system for a game engine.
The functionalities from past homework, including multiplayer and object models, also need
to be implemented in this homework. Especially, networking is closely related to event
management.

Prerequisites
----
* Ubuntu 20.04
    + Desktop system installed
    + Additional packages
        - `x11-apps`
        - `build-essential`
        - `libsfml-dev`
        - `libzmq3-dev`
        - `nlohmann-json3-dev`

Getting Started
----
1. In the root directory of this project, run `make` in command line.
    * If succeed, two executables: `server` and `client` should appear in the directory.
2. run `./server` in any terminal and then run `./client` in a terminal which is opened from the Ubuntu desktop.
    * If succeed, a 800 x 600 window should appear and there will be a white square which represents the character.
    * Use the four direction keys to move around.
    * Use the `R` key to start and stop recording. The replaying starts when the recording stops.
    * When your character is collided with other players' characters, it is killed and respawned from random position.
    * The goal is to survive as long as possible.
