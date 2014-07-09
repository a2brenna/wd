CXX=clang++
CXXFLAGS=-O2 -g -std=c++11 -fPIC -Wall -Wextra
DESTDIR=/
PREFIX=/usr/

.PHONY: proto

all: watchdog/watchdog_pb2.py pb

pb: src/pb.cc
	${CXX} ${CXXFLAGS} src/pb.cc -o pb -lprotobuf -lpthread -lstdc++

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

