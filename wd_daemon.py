#!/usr/bin/python

import signal, sys, socket, select, watchdog_pb2, time, numpy, os, pprint, jarvis_pb2, pwd, pickle, logging, comm
from heartbeat import beat

RECV_BUFF_SIZE=4096
INTERVALS = 100
CONFIDENCE = 5.0

global tasks
tasks = {}
global next_expiration
next_expiration = None
global beat_time

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

def dump_state(tasks):
    try:
        with open(os.path.expanduser("~/.wd.state"), 'wb', 0) as f:
            pickle.dump(tasks, f)
        logging.debug("Dumped state")
    except:
        logging.warning("Failed to dump state")

def awake(signum, frame):
    global next_expiration
    global tasks
    global beat_time
    logging.debug("Awake")
    try:
        if (time.time() - beat_time > 60.0):
            logging.debug("Beating")
            beat(server=wd_server, port=wd_port, signature='wd:primary')
            beat_time = time.time()
    except:
        logging.warning("Failed to contact wd server")

    if next_expiration != None:
        logging.debug("Checking next_expiration")
        if next_expiration.expiration < time.time():
            logging.debug("Task: " + next_expiration.signature + " has expired")
            try:
                logging.debug("Sending expiration notice")
                comm.send_jarvis("wd." + socket.gethostname(), pwd.getpwuid( os.getuid() )[ 0 ], next_expiration.signature + " has expired.")
            except:
                logging.error("Failed to send expiration notice for " + str(next_expiration.signature))
            dump_state(tasks)
    try:
        current_time = time.time()
        next_expiration = min([t for t in tasks.values() if (get_exp(t) > current_time)] , key=get_exp)
        signal.setitimer(signal.ITIMER_REAL, min(next_expiration.expiration - time.time(), beat_time + 60 - time.time()))
        logging.info("Next expiration is " + next_expiration.signature + " @ " + str(next_expiration.expiration))
    except:
        next_expiration = None
        signal.setitimer(signal.ITIMER_REAL, max(beat_time + 60.0 - time.time(), 0.001))
        logging.info("No pending expiration")

def daemon(port, dumpdir, wd_server, wd_port):
    global tasks
    global beat_time

    logging.basicConfig(filename=os.path.expanduser("~/.wd.log"), level=logging.DEBUG, format='%(asctime)s: %(levelname)s: %(message)s')
    logging.info("Secondary watchdog server: " + wd_server + ":" + str(wd_port))

    try:
        with open(os.path.expanduser("~/.wd.state"), 'rb', 0) as f:
            tasks = dict(pickle.load(f))
            logging.debug("Loaded state from ~/.wd.state")
    except:
        tasks = {}
        logging.debug("Could not load state from ~/.wd.state")

    beat_time = time.time()
    try:
        beat(server=wd_server, port=wd_port, signature='wd:primary')
    except:
        logging.warning("Failed to contact wd server " + wd_server + ":" + str(wd_port))

    #TODO: investigate why this doesn't work as expected
    #signal.siginterrupt(signal.SIGALRM, True)
    signal.signal(signal.SIGALRM, awake)

    try:
        inet = socket.socket(socket.AF_INET)
        inet.bind(('', port))
        inet.listen(1)
        logging.debug("Now listening on port " + str(port))
    except:
        logging.critical("Failed to open socket.\n")

    while True:
        try:
            if (time.time() - beat_time > 60.0):
                beat(server=wd_server, port=wd_port, signature='wd:primary')
                beat_time = time.time()
        except:
            logging.warning("Failed to contact wd server" + wd_server + ":" + str(wd_port))
        try:
            for x in select.select([inet],[],[],1)[0]: #Readable sockets returned by select

                if x == inet:
                    c, client_addr = x.accept()
                    data = c.recv(RECV_BUFF_SIZE)
                    message = watchdog_pb2.Message()
                    try:
                        message.ParseFromString(data)
                    except:
                        logging.warning("Failed to parse incoming message, attempting compatability behaviour")
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
                            logging.debug("Received beat: " + str(message.beat.signature))
                            awake(signal.SIGALRM, None)
                            dump_state(tasks)
                        elif message.HasField('query'):
                            logging.debug("Received query: " + str(message.query.question))
                            response = watchdog_pb2.Message()
                            for s, t in tasks.iteritems():
                                description = response.response.task.add()
                                description.signature = s
                                description.last = int(t.heartbeats[-1])
                                description.expected = int(t.expiration)
                            c.send(response.SerializeToString())
                        else:
                            logging.error("Unhandled Message: " + str(message))
                    else:
                        #unparseable message...
                        logging.error("Uninitialized Message: " + str(message))
                    c.close()
                #awake(signal.SIGALRM, None)
        except KeyboardInterrupt: #ALSO CATCHES SIGINT
            pass
        except select.error:
            pass
