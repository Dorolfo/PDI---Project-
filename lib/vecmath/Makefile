# Makefile for vecmath library (outputs into lib/)

CXX      := g++
CXXFLAGS := -Wall -Wextra -O2 -std=c++17 -Iinclude

SRC_DIR  := src
OBJ_DIR  := obj
INC_DIR  := include
LIB_DIR  := lib

SRC      := $(wildcard $(SRC_DIR)/*.cpp)
OBJ      := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC))

LIB_NAME := $(LIB_DIR)/libvecmath.a

all: $(LIB_NAME)

# Archive into lib/
$(LIB_NAME): $(OBJ) | $(LIB_DIR)
	ar rcs $@ $^

# Compile .cpp → .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Create folders if missing
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

clean:
	rm -rf $(OBJ_DIR) $(LIB_DIR)

.PHONY: all clean

