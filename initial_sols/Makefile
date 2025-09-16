# Makefile mejorado para manejo modular de métodos de generación de columnas

CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra

# Source files
BASE_SOURCES = ProblemInstance.cpp ColumnGenerationBase.cpp
METHOD_SOURCES = HeuristicMethod.cpp RandomMethod.cpp 
MAIN_SOURCES = Initial_sols.cpp

SOURCES = $(BASE_SOURCES) $(METHOD_SOURCES) $(MAIN_SOURCES)

# Header files
BASE_HEADERS = ProblemInstance.h ColumnGenerationBase.h
METHOD_HEADERS = HeuristicMethod.h RandomMethod.h 

HEADERS = $(BASE_HEADERS) $(METHOD_HEADERS)

# Object files
BASE_OBJECTS = $(BASE_SOURCES:.cpp=.o)
METHOD_OBJECTS = $(METHOD_SOURCES:.cpp=.o)
MAIN_OBJECTS = $(MAIN_SOURCES:.cpp=.o)

OBJECTS = $(BASE_OBJECTS) $(METHOD_OBJECTS) $(MAIN_OBJECTS)

TARGET = program

# Default target
all: $(TARGET)

# Main executable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(TARGET)

# Base component objects
ProblemInstance.o: ProblemInstance.cpp ProblemInstance.h
	$(CXX) $(CXXFLAGS) -c ProblemInstance.cpp

ColumnGenerationBase.o: ColumnGenerationBase.cpp ColumnGenerationBase.h ProblemInstance.h
	$(CXX) $(CXXFLAGS) -c ColumnGenerationBase.cpp

# Method objects
HeuristicMethod.o: HeuristicMethod.cpp HeuristicMethod.h ColumnGenerationBase.h
	$(CXX) $(CXXFLAGS) -c HeuristicMethod.cpp

RandomMethod.o: RandomMethod.cpp RandomMethod.h ColumnGenerationBase.h
	$(CXX) $(CXXFLAGS) -c RandomMethod.cpp

# Main program object
Initial_sols.o: Initial_sols.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c Initial_sols.cpp

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

# Add new method template (example usage: make add-method METHOD=GreedyMethod)
add-method:
	@echo "Creating template for $(METHOD)..."
	@echo "#ifndef $(shell echo $(METHOD) | tr '[:lower:]' '[:upper:]')_H" > $(METHOD).h
	@echo "#define $(shell echo $(METHOD) | tr '[:lower:]' '[:upper:]')_H" >> $(METHOD).h
	@echo "" >> $(METHOD).h
	@echo "#include \"ColumnGenerationBase.h\"" >> $(METHOD).h
	@echo "" >> $(METHOD).h
	@echo "class $(METHOD) : public ColumnGenerationMethod {" >> $(METHOD).h
	@echo "public:" >> $(METHOD).h
	@echo "    $(METHOD)(ProblemInstance* inst) : ColumnGenerationMethod(inst) {}" >> $(METHOD).h
	@echo "    " >> $(METHOD).h
	@echo "    Column generateColumn(int employee_id) override;" >> $(METHOD).h
	@echo "    std::string getMethodName() const override { return \"$(METHOD)\"; }" >> $(METHOD).h
	@echo "};" >> $(METHOD).h
	@echo "" >> $(METHOD).h
	@echo "#endif" >> $(METHOD).h
	@echo "" > $(METHOD).cpp
	@echo "#include \"$(METHOD).h\"" >> $(METHOD).cpp
	@echo "#include <algorithm>" >> $(METHOD).cpp
	@echo "#include <random>" >> $(METHOD).cpp
	@echo "" >> $(METHOD).cpp
	@echo "Column $(METHOD)::generateColumn(int employee_id) {" >> $(METHOD).cpp
	@echo "    Column column(employee_id, instance->horizon_length);" >> $(METHOD).cpp
	@echo "    // TODO: Implement your method here" >> $(METHOD).cpp
	@echo "    evaluateColumn(column);" >> $(METHOD).cpp
	@echo "    return column;" >> $(METHOD).cpp
	@echo "}" >> $(METHOD).cpp
	@echo "Template files $(METHOD).h and $(METHOD).cpp created!"
	@echo "Don't forget to add them to the Makefile's METHOD_SOURCES and METHOD_HEADERS"

.PHONY: all test debug clean rebuild add-method