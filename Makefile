# Variables
CXX = g++
CXXFLAGS = --std=c++17 -O2

SRC_DIR = src
# Source files
SRC_FILES = $(SRC_DIR)/main.cpp \
            $(SRC_DIR)/preprocessor.cpp \
            $(SRC_DIR)/generation.cpp \
            $(SRC_DIR)/parsing.cpp \
            $(SRC_DIR)/tokenization.cpp \
            $(SRC_DIR)/optimizer.cpp


# Target executable
TARGET = brick

# Default target
all: $(TARGET)

$(TARGET): $(SRC_FILES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC_FILES)

# Clean up build files
clean:
	rm -rf $(TARGET)

# Phony targets
.PHONY: all clean
