#!/usr/bin/python

import signal, sys, socket, select, watchdog_pb2
import time
import numpy
import os
import pprint

RECV_BUFF_SIZE=4096

tasks = {}

class Task():
    def __init__(self, signature):
        self.signature = signature
        self.expiration = time.time() + 3600 * 24
        self.heartbeats = []

    def beat(self):
        self.heartbeats.append(time.time())
        if len(heartbeats) < 101:
            return
        else:
            intervals = pwise_diff(self.heartbeats[-100:])
            mean = numpy.mean(intervals[-100:])
            std = numpy.std(intervals[-100:])
            self.expiration = (time.time() + (mean + 3.0 * std))
            return

def sig_handler(signum, frame):
    if signum == signal.SIGUSR1:
        import pprint
        pprint.pprint(tasks)
    else:
        sys.exit(0)

def pwise_diff(t):
    return [t[i+1]-t[i] for i in range(len(t)-1)]

def daemon(port):
    signal.signal(signal.SIGUSR1, sig_handler)
    inet = socket.socket(socket.AF_INET)
    inet.bind(('', port))
    inet.listen(1)
    tasks = {}
    next_expiration = ""

    while True:
        try:
            for x in select.select([inet],[],[],next_expiration[1])[0]: #Readable sockets returned by select
                if x == None:
                    if next_expiration[0] != None:
                        print(next_expiration[0] + " has expired.")
                        del tasks[next_expiration[0]]

                        min_expiration = (None, 3600)
                        for x in tasks.keys():
                            if tasks[x] < min_expiration[1]:
                                min_expiration = (x, tasks[x])

                        next_expiration = min_expiration
                    else:
                        continue
                else:
                    c, client_addr = x.accept()
                    data = c.recv(RECV_BUFF_SIZE)
                    beat = watchdog_pb2.Heartbeat()
                    beat.ParseFromString(data)
                    if beat.IsInitialized():
                        sig = b.signature
                        try:
                            t = tasks[sig]
                            t.append(time.time())
                            if len(t) > 101:
                                e = expiration(t)
                                if e < next_expiration[1]:
                                    next_expiration = (sig, e)
                        except KeyError:
                            tasks[sig] = []
                            tasks[sig].append(time.time())
        except KeyboardInterrupt:
            f = open("state.out", 'w')
            f.write("At time: " + str(time.time()) + "\n")
            for t in tasks.values():
                f.write(str(t.signature) + "\n")
                f.write(str(t.expiration) + "\n")
                f.write("Number of beats: " + str(len(t.heartbeats)) + "\n")
                f.write(pprint.pformat(t.heartbeats) + "\n")
            sys.exit(0)
        except:
            continue
