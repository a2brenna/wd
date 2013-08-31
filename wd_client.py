#!/usr/bin/python

import socket, watchdog_pb2

def client(server, target_port):
    inet = socket.socket(socket.AF_INET)
    inet.connect((server, target_port))
    beat = watchdog_pb2.Heartbeat()
    beat.user = 'test_user'
    beat.execed = 'test_application'
    beat.pid = 42
    beat.host = 'test_machine'
    inet.send(beat.SerializeToString())
    inet.close()
