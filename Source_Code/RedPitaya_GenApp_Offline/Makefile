BUILD_DIR = build
EXECUTABLE = $(BUILD_DIR)/gen_app
SRC_DIR = src
INCLUDE_DIR = include
GEN_FILES_DIR = gen_files

SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRC_FILES))

CXX = g++
CXXFLAGS = -std=c++17 -Wall -pedantic -I$(INCLUDE_DIR) `pkg-config --cflags gtkmm-3.0`
LDFLAGS = `pkg-config --libs gtkmm-3.0` -lstdc++fs -lssh

all: $(BUILD_DIR) $(EXECUTABLE)

$(BUILD_DIR):
	@echo "Creating build directory..."
	@mkdir -p $(BUILD_DIR)
	@echo "Copying gen_files..."
	@cp -r $(GEN_FILES_DIR) $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(EXECUTABLE): $(OBJ_FILES)
	@echo "Linking $(EXECUTABLE)..."
	@$(CXX) $(OBJ_FILES) -o $(EXECUTABLE) $(LDFLAGS)
	@echo "The app has been generated in $(BUILD_DIR)."

clean:
	@echo "Cleaning up generated files..."
	@rm -rf $(BUILD_DIR)
	@echo "Cleanup finished."

run: $(EXECUTABLE)
	@echo "Running application..."
	@./$(EXECUTABLE)

.PHONY: all clean run
