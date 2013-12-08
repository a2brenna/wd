all: watchdog/watchdog_pb2.py

watchdog/watchdog_pb2.py: watchdog.proto
	protoc --python_out='watchdog/' watchdog.proto

clean:
	rm -f watchdog/watchdog_pb2.py
	rm -f *.pyc
	rm -rf __pycache__

