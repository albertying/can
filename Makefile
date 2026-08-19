CXX      := g++
CXXFLAGS := -std=c++20 -O2 -Wall -Wextra

HDRS := bits.hpp dbc.hpp decode.hpp candump.hpp

tool: main.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -o $@ main.cpp

clean:
	rm -f tool

.PHONY: clean
