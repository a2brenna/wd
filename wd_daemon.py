#!/usr/bin/python

import signal, sys, socket, select, watchdog_pb2, time, numpy, os, pprint, jarvis_pb2, pwd, pickle, logging
from heartbeat import beat

RECV_BUFF_SIZE=4096
INTERVALS = 100
CONFIDENCE = 5.0

tasks = {}

class BadMessage(Exception):
    def __init(self, message):
        self.message = message

class UninitializedMessage(Exception):
    def __init(self, message):
        self.message = message

class UnhandledMessage(Exception):
    def __init(self, message):
        self.message = message

class Task():
    def __init__(self, signature):
        self.signature = signature
        self.expiration = 2147483646
        self.heartbeats = []

    def beat(self):
        self.heartbeats.append(time.time())
        if len(self.heartbeats) > INTERVALS:
            intervals = get_intervals(self.heartbeats[-INTERVALS:])
            mean = numpy.mean(intervals[-INTERVALS:])
            std = numpy.std(intervals[-INTERVALS:])
            self.expiration = (time.time() + mean + (CONFIDENCE * std))
        return

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

def dump_state():
    try:
        with open(os.path.expanduser("~/.wd.state"), 'wb', 0) as f:
            pickle.dump(tasks, f)
    except:
        log.write(str(time.time()) + ": WARNING: Failed to dump state\n")

def daemon(port, dumpdir, wd_server, wd_port):
    logging.basicConfig(filename=os.path.expanduser("~/.wd.log"), level=logging.DEBUG, format='%(asctime)s: %(levelname)s: %(message)s')
    try:
        inet = socket.socket(socket.AF_INET)
        inet.bind(('', port))
        inet.listen(1)
        logging.debug("Now listening on port " + str(port))
    except:
        logging.critical("Failed to open socket.\n")

    next_expiration = None

    try:
        with open(os.path.expanduser("~/.wd.state"), 'rb', 0) as f:
            tasks = dict(pickle.load(f))
    except:
        tasks = {}

    log = open(os.path.expanduser("~/.wd.log"), 'a', 0)

    beat_time = time.time()
    try:
        beat(server=wd_server, port=wd_port, signature='wd:primary')
    except:
        log.write(str(time.time()) + ": WARNING: Failed to contact wd server" + wd_server + ":" + str(wd_port) +"\n")

    while True:
        try:
            if (time.time() - beat_time > 60.0):
                beat(server=wd_server, port=wd_port, signature='wd:primary')
                beat_time = time.time()
        except:
            log.write(str(time.time()) + ": WARNING: Failed to contact wd server" + wd_server + ":" + str(wd_port) +"\n")
        try:
            for x in select.select([inet],[],[],1)[0]: #Readable sockets returned by select

                if x == inet:
                    c, client_addr = x.accept()
                    data = c.recv(RECV_BUFF_SIZE)
                    message = watchdog_pb2.Message()
                    try:
                        message.ParseFromString(data)
                    except:
                        log.write(str(time.time()) + ": Failed to parse incoming message, Attempting compatability behaviour\n")
                        try:
                            #A stab at backwards compatability...
                            heartbeat = watchdog_pb2.Heartbeat()
                            heartbeat.ParseFromString(data)
                            message.heartbeat.CopyFrom(heartbeat)
                        except:
                            raise BadMessage(message)
                    if message.IsInitialized():
                        if message.HasField('beat'):
                            sig = message.beat.signature
                            try:
                                t = tasks[sig]
                                t.beat()
                            except KeyError:
                                t = Task(sig)
                                t.beat()
                                tasks[sig] = t
                            log.write(str(time.time()) + ": BEAT: " + str(message.beat.signature) + "\n")
                            dump_state()
                        elif message.HasField('query'):
                            log.write(str(time.time()) + ": QUERY\n")
                            response = watchdog_pb2.Message()
                            for s, t in tasks.iteritems():
                                description = response.response.task.add()
                                description.signature = s
                                description.last = int(t.heartbeats[-1])
                                description.expected = int(t.expiration)
                            c.send(response.SerializeToString())
                        else:
                            raise UnhandledMessage(message)
                    else:
                        #unparseable message...
                        raise UninitializedMessage(message)
                    c.close()
            if next_expiration != None:
                if next_expiration.expiration < time.time():
                    try:
                        expiration_notice(next_expiration)
                    except:
                        log.write(str(time.time()) + ": ERROR: Could not expire: " + str(next_expiration.signature) + "\n")
                    dump_state()
            try:
                current_time = time.time()
                next_expiration = min([t for t in tasks.values() if (get_exp(t) > current_time)] , key=get_exp)
                log.write(str(time.time()) + ": INFO: Next expiration is of: " + next_expiration.signature + " at " + str(next_expiration.expiration) + "\n")
            except:
                next_expiration = None
                log.write(str(time.time()) + ": INFO: No next expiration\n")
        except KeyboardInterrupt: #ALSO CATCHES SIGINT
            pass
