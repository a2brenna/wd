all: watchdog_pb2.py

watchdog_pb2.py: watchdog.proto
	protoc --python_out=. watchdog.proto

clean:
	rm watchdog_pb2.py
