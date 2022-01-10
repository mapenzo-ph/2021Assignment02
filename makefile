CXX = mpic++
CXXFLAGS = -std=c++14 -O3

SRC = kdtree.cpp
EXE = $(SRC:.cpp=)

.PHONY:	default clean

default:	$(EXE)

%:	%.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm $(EXE)