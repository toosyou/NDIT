CC=gcc-5
CXX=g++-5
INCLUDE=progressbar/include/
CXXFLAGS=-ltiff -fopenmp -lncurses -I$(INCLUDE) -Lprogressbar/ -lprogressbar

all: neuron_detection_in_tiff

neuron_detection_in_tiff:tomo_tiff.o main.o progressbar/libprogressbar.so
	$(CXX) tomo_tiff.o main.o $(CXXFLAGS) -o neuron_detection_in_tiff

tomo_tiff.o:tomo_tiff.cpp
	$(CXX) $(CXXFLAGS) -c tomo_tiff.cpp -o tomo_tiff.o

main.o:main.cpp
	$(CXX) $(CXXFLAGS) -c main.cpp -o main.o

progressbar/libprogressbar.so:
	cd progressbar && make;

clean:
	rm -f neuron_detection_in_tiff tomo_tiff.o main.o