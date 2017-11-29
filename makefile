all: client.cpp server.cpp
	g++ client.cpp -o client -std=c++11
	g++ server.cpp -o server -std=c++11
clean:
	rm -f client
	rm -f server
