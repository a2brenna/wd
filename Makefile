all: watchdog_pb2.py jarvis_pb2.py

watchdog_pb2.py: watchdog.proto
	protoc --python_out='.' watchdog.proto

jarvis_pb2.py: jarvis/jarvis.proto
	protoc --python_out='.' jarvis/jarvis.proto
	mv jarvis/jarvis_pb2.py .

clean:
	rm watchdog_pb2.py
	rm jarvis_pb2.py

