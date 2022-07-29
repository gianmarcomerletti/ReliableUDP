all: server client


server: server.o util.o
	gcc -o server server.o util.o
server.o: server.c
	gcc -c server.c

client: client.o util.o
	gcc -o client client.o util.o
client.o: client.c
	gcc -c client.c

util.o: util.c
	gcc -c util.c
	

clean:
	rm -f client client.o server server.o util.o