
CC=g++
CFLAGS= -w
LFLAGS= -lcurl -lboost_regex -lboost_system
main: main.cpp main.hpp 
	$(CC) $(CFLAGS) -o main main.cpp $(LFLAGS)

