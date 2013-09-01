#!/usr/bin/python

import socket, watchdog_pb2

def client(server, target_port):
    inet = socket.socket(socket.AF_INET)
    inet.connect((server, target_port))
    beat = watchdog_pb2.Heartbeat()
    beat.signature = 'test_user:host:test_application:42'
    inet.send(beat.SerializeToString())
    inet.close()
