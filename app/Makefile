# Makefile for application ECOI24 - 2025_02
# Prof. Giovani Bernardes Vitor

CXX      := g++

UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
    CXXFLAGS := -Wall -Wextra -O2 -std=c++17 -DGL_SILENCE_DEPRECATION \
                -Iinclude \
                -I../lib/vecmath/include
    LDFLAGS  := -L../lib/vecmath/lib -lvecmath -framework OpenGL -framework GLUT
else
    CXXFLAGS := -Wall -Wextra -O2 -std=c++17 \
                -Iinclude \
                -I../lib/vecmath/include
    LDFLAGS  := -L../lib/vecmath/lib -lvecmath -lGL -lGLU -lglut
endif

SRC_DIR  := src
OBJ_DIR  := obj
BIN      := START_CODE

SRC      := $(wildcard $(SRC_DIR)/*.cpp)
OBJ      := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC))

all: ../vecmath/lib/libvecmath.a $(OBJ) $(BIN)

# Build the app
$(BIN): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) 

# Compile .cpp → .o into obj/
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Create obj/ directory if missing
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Ensure vecmath lib is built first
../vecmath/lib/libvecmath.a:
	$(MAKE) -C ../lib/vecmath

clean:
	rm -rf $(OBJ_DIR) $(BIN)
	$(MAKE) -C ../lib/vecmath clean

.PHONY: all clean

