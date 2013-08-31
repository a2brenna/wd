#!/usr/bin/python3

import socket

def client(server, target_port):
    inet = socket.socket(socket.AF_INET)
    inet.connect((server, target_port))
    inet.send(b'TEST')
    inet.close()
