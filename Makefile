.PHONY: proto

all: watchdog/watchdog_pb2.py

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

