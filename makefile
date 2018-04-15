all: main
# all: main pmain.so

cxx=clang++
llvmcxxflags=$(shell llvm-config --cxxflags)
llvmldflags=$(shell llvm-config --ldflags --system-libs --libs core)
cxxflags= -std=c++14 -g
py_inc=-I/usr/include/python2.7

main.o: main.cpp  token.h parse.h
	$(cxx) -c -fPIC  $(llvmcxxflags) $(cxxflags)  $< -o $@

main: main.o
	$(cxx) $< $(llvmldflags) -o $@

pmain.so: pmain.o main.o
	$(cxx) -shared  $^ -o $@

pmain.cc: pmain.pyx
	cython --cplus $< -o $@

pmain.o: pmain.cc
	$(cxx) -c -fPIC $(py_inc) $< -o $@


clean:
	rm *.so *.cc *.o
