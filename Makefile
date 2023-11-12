OBJS = src/event/event/event.o src/event/event_manager/event_manager.o src/event/timeline/timeline.o src/event/timestamp/timestamp.o src/world/character/character.o src/world/world/world.o
LIBS = -lsfml-graphics -lsfml-window -lsfml-system -lzmq -lpthread
CXXFLAGS = -std=c++17 -Isrc

all: server client

server: src/program/server/server.o $(OBJS)
	$(CXX) -o server src/program/server/server.o $(OBJS) $(LIBS)

client: src/program/client/client.o $(OBJS)
	$(CXX) -o client src/program/client/client.o $(OBJS) $(LIBS)

####

src/event/event/event.o: src/event/event/event.hpp src/event/timestamp/timestamp.hpp src/event/timeline/timeline.hpp

src/event/event_manager/event_manager.o: src/event/event_manager/event_manager.hpp src/event/event/event.hpp src/event/timeline/timeline.hpp src/event/timestamp/timestamp.hpp src/world/world/world.hpp

src/event/timeline/timeline.o: src/event/timeline/timeline.hpp src/event/timestamp/timestamp.hpp

src/event/timestamp/timestamp.o: src/event/timestamp/timestamp.hpp

src/world/character/character.o: src/world/character/character.hpp

src/world/world/world.o: src/world/world/world.hpp src/world/character/character.hpp src/event/timestamp/timestamp.hpp

src/program/client/client.o: src/event/event/event.hpp src/world/world/world.hpp

####

.PHONY: clean
clean:
	rm server client $(OBJS)
