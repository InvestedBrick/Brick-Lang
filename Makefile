# Variables
CXX = g++
CXXFLAGS = --std=c++17 -O2


# Source files
SRC_FILES = src/main.cpp \
            src/preprocessor.cpp \
            src/generation.cpp \
            src/parsing.cpp \
            src/tokenization.cpp



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
