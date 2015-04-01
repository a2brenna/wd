INCLUDE_DIR=$(shell echo ~)/local/include
LIBRARY_DIR=$(shell echo ~)/local/lib
DESTDIR=/
PREFIX=/usr/

CXX=clang++
CXXFLAGS=-L${LIBRARY_DIR} -I${INCLUDE_DIR} -O2 -g -std=c++11 -fPIC -Wall -Wextra

all: pb test

pb: src/pb.cc src/server_config.h watchdog.pb.o task.o server_config.o common_config.o
	${CXX} ${CXXFLAGS} src/pb.cc watchdog.pb.o task.o server_config.o common_config.o -o pb -lprotobuf -lpthread -lstdc++ -lhgutil -lgnutls -lboost_program_options -ljsoncpp -lcurl -lsmplsocket -ltxtable

test: src/test.cc client.o watchdog.pb.o common_config.o client_config.o
	${CXX} ${CXXFLAGS} src/test.cc client.o watchdog.pb.o common_config.o client_config.o -o test -lprotobuf -lpthread -lstdc++ -lhgutil -lgnutls -lcurl -ljsoncpp -lsmplsocket

client.o: src/client.cc src/client.h
	${CXX} ${CXXFLAGS} -c src/client.cc -o client.o

task.o: src/task.cc src/task.h
	${CXX} ${CXXFLAGS} -c src/task.cc -o task.o

server_config.o: src/server_config.cc src/server_config.h
	${CXX} ${CXXFLAGS} -c src/server_config.cc -o server_config.o

client_config.o: src/client_config.cc src/client_config.h
	${CXX} ${CXXFLAGS} -c src/client_config.cc -o client_config.o

common_config.o: src/common_config.cc src/common_config.h
	${CXX} ${CXXFLAGS} -c src/common_config.cc -o common_config.o

watchdog.pb.o: watchdog.proto
	mkdir -p src/
	protoc --cpp_out='src/' watchdog.proto
	${CXX} ${CXXFLAGS} -c src/watchdog.pb.cc -o watchdog.pb.o

clean:
	rm -f watchdog/watchdog_pb2.py
	rm -f *.pyc
	rm -rf __pycache__
	rm -f src/watchdog.pb.cc
	rm -f src/watchdog.pb.h
	rm -f pb
	rm -f test
	rm -f puppy
	rm -f *.o
