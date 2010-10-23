CXXFLAGS = -mno-cygwin -O3

ScaleGfx: ScaleGfx.o ScaleNoSSE.o ScaleSSE.o
	$(CXX) -shared -Wl,--output-def ScaleGfx.def -mno-cygwin -s -o ScaleGfx.dll $^ -lgdi32

ScaleNoSSE.o : CallScale.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^ -DFUNCTION=ScaleNoSSE
ScaleSSE.o : CallScale.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^ -DFUNCTION=ScaleSSE -mfpmath=sse -msse

clean:
	rm -f *.o
