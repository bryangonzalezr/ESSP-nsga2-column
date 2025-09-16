# Makefile mejorado para manejo modular de métodos de generación de columnas

CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra

# Source files
BASE_SOURCES = ProblemInstance.cpp
GA_SOURCES = ScheduleSearch.cpp

MAIN_SOURCES = Initial_sols.cpp

SOURCES = $(BASE_SOURCES) $(GA_SOURCES) $(MAIN_SOURCES)

# Header files
BASE_HEADERS = ProblemInstance.h 
GA_HEADERS = ScheduleSearch.h

HEADERS = $(BASE_HEADERS) $(GA_HEADERS)

# Object files
BASE_OBJECTS = $(BASE_SOURCES:.cpp=.o)
GA_OBJECTS = $(GA_SOURCES:.cpp=.o)
MAIN_OBJECTS = $(MAIN_SOURCES:.cpp=.o)

OBJECTS = $(BASE_OBJECTS) $(GA_OBJECTS) $(MAIN_OBJECTS)

TARGET = program

# Default target
all: $(TARGET)

# Main executable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(TARGET)

# Base component objects
ProblemInstance.o: ProblemInstance.cpp ProblemInstance.h
	$(CXX) $(CXXFLAGS) -c ProblemInstance.cpp

# Main program object
Initial_sols.o: Initial_sols.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c Initial_sols.cpp

ScheduleSearch.o: ScheduleSearch.cpp ScheduleSearch.h
	$(CXX) $(CXXFLAGS) -c ScheduleSearch.cpp

# Test target for comparing methods
test: $(TARGET)
	./$(TARGET)

# Debug version
debug: CXXFLAGS += -g -DDEBUG
debug: $(TARGET)

# Clean target
clean:
	rm -f $(TARGET) $(OBJECTS)

# Rebuild everything
rebuild: clean all
