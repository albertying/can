CXX      := g++
CXXFLAGS := -std=c++20 -O2 -Wall -Wextra

tool: main.cpp
	$(CXX) $(CXXFLAGS) -o $@ main.cpp

clean:
	rm -f tool

.PHONY: clean
