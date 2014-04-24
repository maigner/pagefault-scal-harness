
harness: main.cc
	g++ -c -O0 -std=c++11 -pthread main.cc
	g++ -O0 -std=c++11 -o pagefault-scal-harness main.o -pthread

clean:
	rm pagefault-scal-harness
