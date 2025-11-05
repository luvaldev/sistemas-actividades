# makefile para compilar el proyecto
# usa c++17 y habilita los threads
CXX = g++
CXXFLAGS = -std=c++17 -pthread -Wall -Wextra

# archivos fuente
SRCS = src/main.cpp src/config.cpp src/hero.cpp src/monster.cpp

# nombre del ejecutable final
TARGET = doom_sim

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)

# para ejecutarlo rapidamente
run: all
	./$(TARGET) ejemplo1.txt

run2: all
	./$(TARGET) ejemplo2.txt

run3: all
	./$(TARGET) ejemplo3.txt