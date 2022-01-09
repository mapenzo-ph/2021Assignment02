CXX = mpic++
CXXFLAGS = 

SRC = kdtree.cpp
EXE = $(SRC:.cpp=.x)

.PHONY:	default clean

default:	$(EXE)

%.x:	%.cpp
	$(CXX) -o $@ $<

clean:
	rm $(EXE)