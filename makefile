all: main pmain.so

cxx=clang++
cxxflags= -std=c++14 -g
py_inc=-I/usr/include/python2.7

main.o: main.cpp  token.h parse.h
	$(cxx) -c -fPIC  $(cxxflags) $< -o $@

main: main.o
	$(cxx) $< -o$@

pmain.so: pmain.o main.o
	$(cxx) -shared  $^ -o $@

pmain.cc: pmain.pyx
	cython --cplus $< -o $@

pmain.o: pmain.cc
	$(cxx) -c -fPIC $(py_inc) $< -o $@


clean:
	rm *.so *.cc *.o
