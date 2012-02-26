
all: 
	gcc tracker.c -g -o tracker -lpthread -std=gnu99 
	gcc tgen.c -g -o tgen
	gcc peer.c -g -o peer   -lpthread
	gcc peerlist.c -g -o peerlist   -lpthread -std=gnu99 
clean:
	rm tracker tgen peerlist peer
