CXX = g++-11
CXXFLAGS = -std=c++14 -fopenmp 
SRC = omp_kdtree.cpp
EXE = $(SRC:.cpp=)

PYC = python3
PYSRC = test_data.py
PYOUT = $(PYSRC:.py=.csv)

.PHONY:	default clean

default:	$(PYOUT) $(EXE)

%:	%.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

%.csv:	%.py
	$(PYC) $< $@ 3 50

clean:
	rm $(EXE)