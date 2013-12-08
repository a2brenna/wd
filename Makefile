all: watchdog_pb2.py

watchdog_pb2.py: watchdog.proto
	protoc --python_out='.' watchdog.proto

clean:
	rm -f watchdog_pb2.py
	rm -f *.pyc
	rm -rf __pycache__

