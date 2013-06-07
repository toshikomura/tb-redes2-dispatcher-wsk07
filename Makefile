all:
	@g++ socket.cpp client.cpp protocol.cpp -o client -ggdb
	@g++ socket.cpp server.cpp protocol.cpp -o server -ggdb
