# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -I./include

# Directories
SRC_DIR = src
BIN_DIR = bin
OBJ_DIR = obj
INCLUDE_DIR = include

# Source files and object files
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# Output binary
TARGET = $(BIN_DIR)/node

# Default target to build the program
all: $(TARGET)

# Link the object files to create the binary
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET)

# Compile each .cpp file into an object file
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)  # Create the obj directory if it doesn't exist
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean object and binary files
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)/main

# Run the program
run: $(TARGET)
	$(TARGET)

.PHONY: all clean run
