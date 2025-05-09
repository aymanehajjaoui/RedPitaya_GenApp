BUILD_DIR = build
SRC_DIR = src
INCLUDE_DIR = include
GEN_FILES_DIR = gen_files

EXECUTABLE = $(BUILD_DIR)/gen_app

SRC_FILES = $(shell find $(SRC_DIR) -name '*.cpp')
OBJ_FILES = $(patsubst $(SRC_DIR)/%, $(BUILD_DIR)/%, $(SRC_FILES:.cpp=.o))

CXX = g++
CXXFLAGS = -std=c++17 -Wall -pedantic -I$(INCLUDE_DIR) `pkg-config --cflags gtkmm-3.0`
LDFLAGS = `pkg-config --libs gtkmm-3.0` -lstdc++fs -lssh

all: clean appbuild run

$(BUILD_DIR):
	@echo "Creating build directory..."
	@mkdir -p $(BUILD_DIR)
	@if [ ! -d $(GEN_FILES_DIR) ]; then \
		echo "Error: $(GEN_FILES_DIR) folder not found. Please ensure it exists."; \
		exit 1; \
	fi
	@echo "Copying gen_files into build/..."
	@cp -r $(GEN_FILES_DIR) $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

appbuild: $(BUILD_DIR) $(EXECUTABLE)

$(EXECUTABLE): $(OBJ_FILES)
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
