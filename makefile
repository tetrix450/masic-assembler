all: main.cpp
	g++ main.cpp -o emasic -Wall

update: emasic
	sudo cp ./emasic /usr/local/bin
	sudo chmod +x /usr/local/bin/emasic