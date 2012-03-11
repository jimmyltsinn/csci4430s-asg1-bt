all: peer tracker tgen

sort.o: sort.c
	gcc sort.c -c -g -o sort.o

peer_basic.o: peer_basic.c peer.h
	gcc peer_basic.c -c -g -o peer_basic.o

peer_file.o: peer_file.c peer.h
	gcc peer_file.c -c -g -o peer_file.o

peer_tracker.o: peer_tracker.c peer.h
	gcc peer_tracker.c -c -g -o peer_tracker.o

peer_peer.o: peer_peer.c peer.h
	gcc peer_peer.c -c -g -o peer_peer.o

peer_main.o: peer_main.c peer.h
	gcc peer_main.c -c -g -o peer_main.o

peer.o: peer.c peer.h
	gcc peer.c -c -g -lpthread -o peer.o

peer: peer_tracker.o peer_basic.o peer_peer.o peer_main.o sort.o peer.c peer.h
	gcc peer_tracker.o peer.o peer_basic.o peer_peer.o peer_main.o sort.o -o peer

tracker: tracker.c
	gcc tracker.c -g -o tracker -lpthread -std=gnu99

tgen: tgen.c
	gcc tgen.c -g -o tgen

clean:
	rm -f tracker tgen *.o peer 
