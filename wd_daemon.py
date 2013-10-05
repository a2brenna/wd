#!/usr/bin/python

import signal, sys, socket, select, watchdog_pb2, time, numpy, os, jarvis_pb2, pwd, pickle, logging, comm, traceback
from heartbeat import beat

RECV_BUFF_SIZE=4096
MIN_INTERVALS = 100
INTERVALS = 10000
CONFIDENCE = 5.0

global WD_SERVER
WD_SERVER = ""
global WD_PORT
WD_PORT = 0

global tasks
tasks = {}
global next_expiration
next_expiration = None
global beat_time
beat_time = 0

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
        self.ivals= []
        self.last = time.time()

    def beat(self):
        current = time.time()
        self.ivals.append(current - self.last)
        self.last = current
        self.heartbeats.append(time.time())
        if len(self.heartbeats) > MIN_INTERVALS:
            intervals = self.intervals()
            mean = numpy.mean(intervals)
            std = numpy.std(intervals)
            self.expiration = (time.time() + mean + (CONFIDENCE * std))
        return

    def intervals(self):
        return get_intervals(self.heartbeats[-INTERVALS:])

    def deviation(self):
        intervals = self.intervals()
        if len(intervals) > 2:
            return numpy.std(intervals)
        else:
            return 0

    def mean(self):
        intervals = self.intervals()
        if len(intervals) > 2:
            return numpy.mean(intervals)
        else:
            return 0

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
    except Exception as e:
        logging.warning("Failed to dump state")
        logging.exception(e)

def awake(signum, frame):
    global next_expiration
    global tasks
    global beat_time
    global WD_SERVER
    global WD_PORT
    logging.debug("Awake")
    try:
        if (time.time() - beat_time > 60.0):
            logging.debug("Beating")
            beat(server=WD_SERVER, port=WD_PORT, signature='wd:primary')
            beat_time = time.time()
    except Exception as e:
        logging.warning("Failed to contact wd server")
        logging.exception(e)

    if next_expiration != None:
        logging.debug("Checking next_expiration")
        if next_expiration.expiration < time.time():
            logging.debug("Task: " + next_expiration.signature + " has expired")
            try:
                logging.debug("Sending expiration notice")
                comm.send_jarvis("wd." + socket.gethostname(), pwd.getpwuid( os.getuid() )[ 0 ], next_expiration.signature + " has expired.")
            except Exception as e:
                logging.error("Failed to send expiration notice for " + str(next_expiration.signature))
                logging.exception(e)
            dump_state(tasks)
    try:
        current_time = time.time()
        next_expiration = min([t for t in tasks.values() if (get_exp(t) > current_time)] , key=get_exp)
        signal.setitimer(signal.ITIMER_REAL, min(next_expiration.expiration - time.time(), beat_time + 60 - time.time()))
        logging.info("Next expiration is " + next_expiration.signature + " @ " + str(next_expiration.expiration))
    except Exception as e:
        next_expiration = None
        signal.setitimer(signal.ITIMER_REAL, max(beat_time + 60.0 - time.time(), 0.001))
        logging.info("No pending expiration")
        logging.exception(e)

def log_uncaught(ex_cls, ex, tb):
    logging.critical("Unhandled Exception!")
    trace_string = ''.join(traceback.format_tb(tb))
    logging.critical(trace_string)
    exception_string = '{0}: {1}'.format(ex_cls, ex)
    logging.critical(exception_string)
    msg = "WD on " + socket.gethostname() + " has failed\n"
    msg = msg + trace_string + "\n"
    msg = msg + exception_string + "\n"
    comm.send_email(target='a2brenna@csclub.uwaterloo.ca', subject='WD FAILURE', sender='Watchdog', message=msg)

def daemon(port, dumpdir, wd_server, wd_port):
    global tasks
    global beat_time

    global WD_SERVER
    WD_SERVER = wd_server
    global WD_PORT
    WD_PORT = wd_port

    logging.basicConfig(filename=os.path.expanduser("~/.wd.log"), level=logging.DEBUG, format='%(asctime)s: %(levelname)s: %(message)s')
    sys.excepthook = log_uncaught
    logging.info("Secondary watchdog server: " + wd_server + ":" + str(wd_port))

    try:
        with open(os.path.expanduser("~/.wd.state"), 'rb', 0) as f:
            tasks = dict(pickle.load(f))
            logging.debug("Loaded state from ~/.wd.state")
    except Exception as e:
        tasks = {}
        logging.debug("Could not load state from ~/.wd.state")
        logging.exception(e)

    beat_time = time.time()
    try:
        beat(server=wd_server, port=wd_port, signature='wd:primary')
    except Exception as e:
        logging.warning("Failed to contact wd server " + wd_server + ":" + str(wd_port))
        logging.exception(e)

    #TODO: investigate why this doesn't work as expected
    #signal.siginterrupt(signal.SIGALRM, True)
    signal.signal(signal.SIGALRM, awake)

    try:
        inet = socket.socket(socket.AF_INET)
        inet.bind(('', port))
        inet.listen(1)
        logging.debug("Now listening on port " + str(port))
    except Exception as e:
        logging.critical("Failed to open socket.\n")
        logging.exception(e)

    while True:
        try:
            if (time.time() - beat_time > 60.0):
                beat(server=wd_server, port=wd_port, signature='wd:primary')
                beat_time = time.time()
        except Exception as e:
            logging.warning("Failed to contact wd server" + wd_server + ":" + str(wd_port))
            logging.exception(e)
        try:
            for x in select.select([inet],[],[],1)[0]: #Readable sockets returned by select

                if x == inet:
                    logging.debug("Incoming connection")
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
                            logging.debug("Received beat: " + str(message.beat.signature) + "from: " + str(client_addr))
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
                                description.mean = float(t.mean())
                                description.deviation = float(t.deviation())
                            c.send(response.SerializeToString())
                            awake(signal.SIGALRM, None)
                        elif len(message.orders) > 0:
                            logging.debug("Received orders")
                            for cmd in message.orders:
                                for fgt in cmd.to_forget:
                                    try:
                                        del tasks[fgt.signature]
                                    except Exception as e:
                                        logging.warning("Failed to delete: " + fgt.signature)
                                        logging.exception(e)
                            awake(signal.SIGALRM, None)
                            dump_state(tasks)
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
        except BadMessage as e:
            logging.warning("Received bad message")
            logging.exception(e)
