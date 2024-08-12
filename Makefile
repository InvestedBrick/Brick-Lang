# Variables
CXX = g++
CXXFLAGS = --std=c++17

# Directories
INCLUDE_DIR = headers
SRC_DIR = .
OBJ_DIR = obj

# Source files
SRC_FILES = main.cpp \
            preprocessor.cpp \
            generation.cpp \
            parsing.cpp \
            tokenization.cpp

# Object files
OBJ_FILES = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_FILES))

# Target executable
TARGET = main

# Default target
all: $(TARGET)

# Link object files to create the final executable
$(TARGET): $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile source files into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# Create object directory if it doesn't exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Clean up build files
clean:
	rm -rf $(OBJ_DIR) $(TARGET)

# Phony targets
.PHONY: all clean
