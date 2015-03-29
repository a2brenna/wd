INCLUDE_DIR=$(shell echo ~)/local/include
LIBRARY_DIR=$(shell echo ~)/local/lib
DESTDIR=/
PREFIX=/usr/

CXX=clang++
CXXFLAGS=-L${LIBRARY_DIR} -I${INCLUDE_DIR} -O2 -g -std=c++11 -fPIC -Wall -Wextra

all: pb puppy

pb: src/pb.cc src/config.h watchdog.pb.o task.o config.o
	${CXX} ${CXXFLAGS} src/pb.cc watchdog.pb.o task.o config.o -o pb -lprotobuf -lpthread -lstdc++ -lhgutil -lgnutls -lboost_program_options -ljsoncpp -lcurl -lsmplsocket

puppy: src/puppy.cc client.o watchdog.pb.o
	${CXX} ${CXXFLAGS} src/puppy.cc client.o watchdog.pb.o -o puppy -lprotobuf -lpthread -lstdc++ -lhgutil -lgnutls -lcurl -ljsoncpp -lsmplsocket

client.o: src/client.cc src/client.h
	${CXX} ${CXXFLAGS} -c src/client.cc -o client.o

task.o: src/task.cc src/task.h
	${CXX} ${CXXFLAGS} -c src/task.cc -o task.o

config.o: src/config.cc src/config.h
	${CXX} ${CXXFLAGS} -c src/config.cc -o config.o

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
	rm -f puppy
	rm -f *.o
