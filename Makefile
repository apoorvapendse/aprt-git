# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -ggdb -Wall

# Libraries
LIBS = -lssl -lcrypto

# Source and output
SRC = main.cpp
OUT = a.out

# Default target
all: $(OUT)

# Build rule
$(OUT): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) $(LIBS) -o $(OUT)

# Run target
run: $(OUT)
	./$(OUT)

# Clean target
clean:
	rm -f $(OUT)


# TODO: add a target that cleans test dir .git
clean_objects:
	rm -rf ./test/.aprt-git
