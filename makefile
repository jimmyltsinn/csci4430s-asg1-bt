all: peer tracker tgen

peer_file: peer_file.c peer.h
	gcc peer_file.c -c -g -o peer_file.o

peer_tracker: peer_tracker.c peer.h
	gcc peer_tracker.c -c -g -o peer_tracker.o

peer: peer_tracker.o peer_file.o peer.c peer.h
	gcc peer.c -c -g -lpthread -o peer.o
	gcc peer_tracker.o peer_file.o peer.o -o peer

tracker: tracker.c
	gcc tracker.c -g -o tracker -lpthread -std=gnu99

tgen: tgen.c
	gcc tgen.c -g -o tgen

clean:
	rm -f tracker tgen *.o peer 
