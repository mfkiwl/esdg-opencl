CXX = clang++
CXXFLAGS = -g -c
INC = -I/opt/esdg-opencl/include -I/usr/include/x86_64-linux-gnu/c++/4.7 -I/usr/include/x86_64-linux-gnu/c++/4.8 -I../SDKUtil/include
LIBS = -L/opt/esdg-opencl/lib -lOpenCL -lSDKUtil
GLIBS = -lglut -lGLEW -lGL

mandelbrot-display : CXXFLAGS += -DDISPLAY

mandelbrot: Mandelbrot.o MandelbrotDisplay.o
	$(CXX) Mandelbrot.o MandelbrotDisplay.o $(LIBS) -o $@

mandelbrot-display: Mandelbrot.o MandelbrotDisplay.o
	$(CXX) Mandelbrot.o MandelbrotDisplay.o $(LIBS) $(GLIBS) -o $@

Mandelbrot.o: Mandelbrot.cpp
	$(CXX) $(CXXFLAGS) $(INC) $< -o $@

MandelbrotDisplay.o: MandelbrotDisplay.cpp
	$(CXX) $(CXXFLAGS) $(INC) $< -o $@

clean: Mandelbrot.cpp
	rm -rf mandelbrot *.s *.bc binaries microblaze *.dat *.csv *.tmp* init.* *.o
