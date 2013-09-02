#!/usr/bin/python

import signal, sys, socket, select, watchdog_pb2, time, numpy, os, pprint

RECV_BUFF_SIZE=4096
INTERVALS = 100
CONFIDENCE = 3.0

tasks = {}

class Task():
    def __init__(self, signature):
        self.signature = signature
        self.expiration = time.time() + 3600 * 24
        self.heartbeats = []

    def beat(self):
        self.heartbeats.append(time.time())
        if len(self.heartbeats) < (INTERVALS + 1):
            return
        else:
            intervals = pwise_diff(self.heartbeats[-INTERVALS:])
            mean = numpy.mean(intervals[-INTERVALS:])
            std = numpy.std(intervals[-INTERVALS:])
            self.expiration = (time.time() + mean + (CONFIDENCE * std))
            return

def sig_handler(signum, frame):
    if signum == signal.SIGUSR1:
        for t in tasks.keys():
            pprint.pprint(tasks[t].signature)
            pprint.pprint(tasks[t].expiration)
            pprint.pprint(tasks[t].heartbeats)
    else:
        sys.exit(0)

def pwise_diff(t):
    return [t[i+1]-t[i] for i in range(len(t)-1)]

def get_exp(t):
    return t.expiration

def daemon(port):
    signal.signal(signal.SIGUSR1, sig_handler)
    inet = socket.socket(socket.AF_INET)
    inet.bind(('', port))
    inet.listen(1)

    next_expiration = None

    while True:
        try:
            for x in select.select([inet],[],[],1)[0]: #Readable sockets returned by select

                if x == inet:
                    c, client_addr = x.accept()
                    data = c.recv(RECV_BUFF_SIZE)
                    beat = watchdog_pb2.Heartbeat()
                    beat.ParseFromString(data)
                    if beat.IsInitialized():
                        sig = beat.signature
                        try:
                            t = tasks[sig]
                            t.beat()
                        except KeyError:
                            t = Task(sig)
                            t.beat()
                            tasks[sig] = t
            if next_expiration != None:
                if next_expiration.expiration > time.time():
                    continue
                else:
                    print(next_expiration.signature + " has expired.")
                    del tasks[next_expiration.signature]
            try:
                next_expiration = min(tasks.values(), key=get_exp)
            except:
                next_expiration = None
        except KeyboardInterrupt: #ALSO CATCHES SIGINT
            f = open("state.out", 'w')
            f.write("At time: " + str(time.time()) + "\n")
            f.write("Next expiration at " + str(next_expiration.expiration) + " of " + next_expiration.signature)
            for t in tasks.values():
                f.write(str(t.signature) + "\n")
                f.write(str(t.expiration) + "\n")
                f.write("Number of beats: " + str(len(t.heartbeats)) + "\n")
                f.write(pprint.pformat(t.heartbeats) + "\n")
