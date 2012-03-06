all: 
	gcc tracker.c -g -o tracker -lpthread -std=gnu99 
	gcc tgen.c -g -o tgen
	gcc peer.c -g -o peer   -lpthread
	gcc peerlist.c -g -o peerlist   -lpthread -std=gnu99 
	gcc bt_peer.c -g -o bt_peer -lpthread
	gcc bt_open_torrent.c -g -o bt_open -lpthread

clean:
	rm tracker tgen peerlist peer bt_peer bt_open
