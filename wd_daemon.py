#!/usr/bin/python

import signal, sys, socket, select, watchdog_pb2, time, numpy, os, pprint, jarvis_pb2, pwd, pickle, heartbeat

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
            intervals = get_intervals(self.heartbeats[-INTERVALS:])
            mean = numpy.mean(intervals[-INTERVALS:])
            std = numpy.std(intervals[-INTERVALS:])
            self.expiration = (time.time() + mean + (CONFIDENCE * std))
            return

def dump(tasks, next_expiration):
    s = ""
    s += ("Time: " + str(time.time()) + "\n")
    if next_expiration != None:
        s += ("Expiration: " + str(next_expiration.expiration) + " : " + next_expiration.signature + "\n")
    for t in tasks.values():
        s += ("\tExpiration: " + str(t.expiration) + "\n")
        s += ("\tHeartbeats: " + str(len(t.heartbeats)) + "\n")
        s += (pprint.pformat(t.heartbeats) + "\n\n")
    return s


def get_intervals(t):
    return [t[i+1]-t[i] for i in range(len(t)-1)]

def get_exp(t):
    return t.expiration

def expiration_notice(t):
    message = jarvis_pb2.Message()
    message.message = t.signature + " has expired."
    message.sender = "wd." + socket.gethostname()
    message.target = pwd.getpwuid( os.getuid() )[ 0 ]

    jarvis = socket.socket(socket.AF_INET)
    jarvis.connect(('taurine.uwaterloo.ca', 7878))
    jarvis.send(message.SerializeToString())
    jarvis.close()

def daemon(port, dumpdir, wd_server, wd_port):
    inet = socket.socket(socket.AF_INET)
    inet.bind(('', port))
    inet.listen(1)

    next_expiration = None

    try:
        with open(os.path.expanduser("~/.wd.state"), 'rb', 0) as f:
            tasks = dict(pickle.load(f))
    except:
        tasks = {}


    log = open(os.path.expanduser("~/.wd.log"), 'a', 0)

    beat_time = time.time()

    while True:
        try:
            if (time.time() - beat_time > 1.0):
                heartbeat.beat(server=wd_server, port=wd_port)
        except:
            log.write(str(time.time()) + ": WARNING: Failed to contact wd server" + wd_server + ":" + str(wd_port) +"\n")
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
                        log.write(str(time.time()) + ": BEAT: " + str(beat.signature) + "\n")
                        try:
                            with open(os.path.expanduser("~/.wd.state"), 'wb', 0) as f:
                                pickle.dump(tasks, f)
                        except:
                            pass
            if next_expiration != None:
                if next_expiration.expiration > time.time():
                    continue
                else:
                    expiration_notice(next_expiration)
                    del tasks[next_expiration.signature]
            try:
                next_expiration = min(tasks.values(), key=get_exp)
            except:
                next_expiration = None
        except KeyboardInterrupt: #ALSO CATCHES SIGINT
            f = open(dumpdir + "/wd." + str(os.getpid()) + "." + str(time.time()), 'w')
            f.write(dump(tasks, next_expiration))
            f.close()
