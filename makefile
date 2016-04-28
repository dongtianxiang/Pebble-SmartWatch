CXX = gcc

all: server_self.o
	$(CXX) -o server server_self.o

server_self.o: server_self.c
	$(CXX) -c server_self.c


clean:
	rm -rf *.o dchat