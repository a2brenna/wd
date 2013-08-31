#!/usr/bin/python3

import socket, watchdog_pb2

def client(server, target_port):
    inet = socket.socket(socket.AF_INET)
    inet.connect((server, target_port))
    inet.send(b'TEST')
    inet.close()
