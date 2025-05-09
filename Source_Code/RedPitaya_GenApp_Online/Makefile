# Directories
BUILD_DIR = build
SRC_DIR = src
INCLUDE_DIR = include

# Executable name
EXECUTABLE = $(BUILD_DIR)/gen_app

# Source and object files
SRC_FILES = $(shell find $(SRC_DIR) -name '*.cpp')
OBJ_FILES = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRC_FILES))

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -pedantic -I$(INCLUDE_DIR) `pkg-config --cflags gtkmm-3.0`
LDFLAGS = `pkg-config --libs gtkmm-3.0` -lstdc++fs -lssh

all: clean appbuild run

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

appbuild: $(OBJ_FILES)
	@echo "Linking $(EXECUTABLE)..."
	@$(CXX) $(OBJ_FILES) -o $(EXECUTABLE) $(LDFLAGS)
	@echo "The app has been generated in $(BUILD_DIR)."

run:
	@echo "Running application..."
	@./$(EXECUTABLE)

clean:
	@echo "Cleaning up generated files..."
	@rm -rf $(BUILD_DIR)
	@echo "Cleanup finished."

.PHONY: all clean run appbuild
