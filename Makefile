all:
	g++ map_server.cpp -o map_server -levent
	gcc event_client.c -o event_client -lpthread
