#!/usr/bin/python

import signal, sys, socket, select, watchdog_pb2
import time
import numpy

RECV_BUFF_SIZE=4096

def exit():
    sys.exit(0)

def sig_handler(signum, frame):
    if signum == signal.SIGUSR1:
        import pprint
        pprint.pprint(tasks)
    else:
        exit()

def extract_sig(b):
    return (b.signature)

def expiration(t):
    intervals = pwise_diff(t[-100:])
    mean = numpy.mean(intervals[-100:])
    std = numpy.std(intervals[-100:])
    e = (time.time() + (mean + 3.0 * std))
    return e

tasks = {}

def pwise_diff(t):
    return [t[i+1]-t[i] for i in range(len(t)-1)]

def daemon(port):
    signal.signal(signal.SIGUSR1, sig_handler)
    inet = socket.socket(socket.AF_INET)
    inet.bind(('', port))
    inet.listen(1)
    next_expiration = (None, 3600)

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
                        sig = extract_sig(beat)
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
            exit()
        except:
            continue
