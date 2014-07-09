CXX=clang++
CXXFLAGS=-O2 -g -std=c++11 -fPIC -Wall -Wextra
DESTDIR=/
PREFIX=/usr/

.PHONY: proto

all: watchdog/watchdog_pb2.py pb

pb: src/pb.cc src/watchdog.pb.cc src/watchdog.pb.h pitbull.o
	${CXX} ${CXXFLAGS} src/pb.cc src/watchdog.pb.cc pitbull.o -o pb -lprotobuf -lpthread -lstdc++ -lhgutil

pitbull.o: src/pitbull.cc src/pitbull.h
	${CXX} ${CXXFLAGS} -c src/pitbull.cc -o pitbull.o

proto: watchdog.proto
	mkdir -p src
	protoc --python_out='watchdog/' --cpp_out='src/' watchdog.proto

watchdog/watchdog_pb2.py: proto

src/watchdog.pb.cc: proto

src/watchdog.pb.h: proto

clean:
	rm -f watchdog/watchdog_pb2.py
	rm -f *.pyc
	rm -rf __pycache__
	rm -f src/watchdog.pb.cc
	rm -f src/watchdog.pb.h
	rm -f pb
	rm -f pitbull.o

