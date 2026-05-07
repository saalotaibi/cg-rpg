CXX ?= g++
PKG_CONFIG ?= pkg-config

BASE_CXXFLAGS = -std=c++17 -Wall -O2
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
CXXFLAGS ?= $(shell $(PKG_CONFIG) --cflags glew 2>/dev/null) -DGL_SILENCE_DEPRECATION $(BASE_CXXFLAGS)
LDFLAGS ?= $(shell $(PKG_CONFIG) --libs glew 2>/dev/null) -framework GLUT -framework OpenGL -lm
else
CXXFLAGS ?= $(BASE_CXXFLAGS)
LDFLAGS ?= -lGL -lGLU -lGLEW -lglut -lm
endif

ENTRYPOINTS = src/main.cpp
GAME_SRC    = $(filter-out $(ENTRYPOINTS), $(wildcard src/*.cpp))
GAME_OBJ    = $(GAME_SRC:.cpp=.o)

GAME_BIN     = cozy-room

.PHONY: all clean run install-deps

all: $(GAME_BIN)

install-deps:
ifeq ($(UNAME_S),Darwin)
	@command -v brew >/dev/null 2>&1 || { echo "Homebrew not found. Install from https://brew.sh"; exit 1; }
	brew install pkg-config glew freeglut
else
	@echo "Linux: install GLEW, FreeGLUT, and OpenGL dev headers via your package manager."
	@echo "  Debian/Ubuntu: sudo apt install build-essential libglew-dev freeglut3-dev"
	@echo "  Arch:          sudo pacman -S base-devel glew freeglut"
endif

$(GAME_BIN): $(GAME_OBJ) src/main.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f src/*.o $(GAME_BIN)

run: $(GAME_BIN)
	./$(GAME_BIN)
