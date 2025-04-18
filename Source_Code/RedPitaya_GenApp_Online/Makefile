# Directories
BUILD_DIR = build
SRC_DIR = src
INCLUDE_DIR = include

# Executable name
EXECUTABLE = $(BUILD_DIR)/gen_app

# Source and object files
SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRC_FILES))

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -pedantic -I$(INCLUDE_DIR) `pkg-config --cflags gtkmm-3.0`
LDFLAGS = `pkg-config --libs gtkmm-3.0` -lstdc++fs -lssh

# Main target: clean, build and run
all: clean appbuild run

# Create build directory and copy gen_files
$(BUILD_DIR):
	@echo "Creating build directory..."
	@mkdir -p $(BUILD_DIR)

# Compile source files into object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Separate build target to avoid conflict with $(BUILD_DIR)
appbuild: $(BUILD_DIR) $(EXECUTABLE)

# Link object files to create the executable
$(EXECUTABLE): $(OBJ_FILES)
	@echo "Linking $(EXECUTABLE)..."
	@$(CXX) $(OBJ_FILES) -o $(EXECUTABLE) $(LDFLAGS)
	@echo "The app has been generated in $(BUILD_DIR)."

# Run the app
run:
	@echo "Running application..."
	@./$(EXECUTABLE)

# Clean target
clean:
	@echo "Cleaning up generated files..."
	@rm -rf $(BUILD_DIR)
	@echo "Cleanup finished."

.PHONY: all clean run appbuild
