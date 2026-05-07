CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2
LDFLAGS = -lGL -lGLU -lGLEW -lglut -lm

ENTRYPOINTS = src/main.cpp
GAME_SRC    = $(filter-out $(ENTRYPOINTS), $(wildcard src/*.cpp))
GAME_OBJ    = $(GAME_SRC:.cpp=.o)

GAME_BIN     = cozy-room

.PHONY: all clean run

all: $(GAME_BIN)

$(GAME_BIN): $(GAME_OBJ) src/main.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f src/*.o $(GAME_BIN)

run: $(GAME_BIN)
	./$(GAME_BIN)
