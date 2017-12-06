server: server.o 

	gcc -o server server.o -lpthread

server.o: server.c server.h

	gcc -c server.c

clean:
	rm -f server.o server
