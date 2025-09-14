# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -ggdb -Wall

# Libraries
LIBS = -lssl -lcrypto

# Sources, objects, and output
SRC = $(wildcard *.cpp)
OBJ = $(patsubst %.cpp, build/%.o, $(SRC))
OUT = build/a.out

# Default target
all: $(OUT)

# Link step
$(OUT): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) $(LIBS) -o $(OUT)

# Compile step
build/%.o: %.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Ensure build dir exists
build:
	mkdir -p build

# Run target
run: $(OUT)
	./$(OUT)

# Clean target
clean:
	rm -rf build

# Custom clean target for test dir
clean_objects:
	rm -rf ./test/.aprt-git
