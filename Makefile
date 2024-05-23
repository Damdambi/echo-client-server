all: echo-server echo-client
echo-server : echo-server.c
	gcc -o echo-server echo-server.c
echo-client : echo-client.c
	gcc -o echo-client echo-client.c
clean : 
	rm -rf echo-server echo-client