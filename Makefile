CC=g++
FLAGS=-g -std=c++11 -Wall -pedantic -O0
LIBS=-lpthread

all: http_client http_server

.phony: clean 

http_client: http_client.cpp
	$(CC) $(FLAGS) http_client.cpp -o http_client $(LIBS)

http_server: http_server.cpp
	$(CC) $(FLAGS) http_server.cpp -o http_server $(LIBS)

clean: 
	rm -rf *.o http_client http_server 
