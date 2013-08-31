#!/usr/bin/python

import signal, sys, socket, select, watchdog_pb2
import time

RECV_BUFF_SIZE=4096

class Task():
    def __init__(self, signature):
        self.signature = signature
        self.beats = []

    def beat(self):
        self.beats.append(time.time())

def extract_sig(b):
    return (b.host + ":" + b.user + ":" + b.execed + ":" + str(b.pid))

tasks = {}

def daemon(port):
    inet = socket.socket(socket.AF_INET)
    inet.bind(('', port))
    inet.listen(1)

    while True:
        for x in select.select([inet],[],[],1)[0]: #Readable sockets returned by select
            c, client_addr = x.accept()
            data = c.recv(RECV_BUFF_SIZE)
            beat = watchdog_pb2.Heartbeat()
            beat.ParseFromString(data)
            print(beat)
