all : server.cpp client.cpp
	g++ -std=c++11 -o server server.cpp
	g++ -std=c++11 -o client client.cpp
clean:
	rm -f server client