CXX=g++
CXXFLAGS=-std=c++17 -Wall -Wextra -O2

BIN_DIR=build

all: $(BIN_DIR)/server $(BIN_DIR)/client $(BIN_DIR)/moderator

$(BIN_DIR)/server: src/server.cpp src/common.hpp
	mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) src/server.cpp -o $@

$(BIN_DIR)/client: src/client.cpp src/common.hpp
	mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) src/client.cpp -o $@

$(BIN_DIR)/moderator: src/moderator.cpp src/common.hpp
	mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) src/moderator.cpp -o $@

clean:
	rm -rf $(BIN_DIR)
