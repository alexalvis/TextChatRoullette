client: client.o 

	gcc -o client client.o -lpthread

client.o: client.c client.h

	gcc -c client.c

clean:
	rm -f client.o client
