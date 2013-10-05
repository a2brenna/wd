#!/usr/bin/python

import signal, sys, socket, select, watchdog_pb2, time, numpy, os, jarvis_pb2, pwd, pickle, logging, comm, traceback
from heartbeat import beat

RECV_BUFF_SIZE=4096
MIN_INTERVALS = 100
INTERVALS = 10000
CONFIDENCE = 5.0

class MessageError(Exception):
    def __init__(self, message, error):
        self.message = message
        self.error = error

class Task():
    def __init__(self, signature):
        self.signature = signature
        self.expiration = 2147483646
        self.ivals= []
        self.last = time.time()

    def beat(self):
        current = time.time()
        self.ivals.append(current - self.last)
        self.last = current
        if len(self.ivals) > MIN_INTERVALS:
            mean = self.mean()
            std = self.deviation()
            self.expiration = (time.time() + mean + (CONFIDENCE * std))
        return

    def deviation(self):
        if len(self.ivals) > 2:
            return numpy.std(self.ivals)
        else:
            return 0

    def mean(self):
        if len(self.ivals) > 2:
            return numpy.mean(self.ivals)
        else:
            return 0

def get_exp(t):
    return t.expiration

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

class WatchDog():

    def __init__(self, port, wd_server, wd_port):
        self.port = port
        self.wd_server = wd_server
        self.wd_port = wd_port

        try:
            with open(os.path.expanduser("~/.wd.state"), 'rb', 0) as f:
                self.tasks = dict(pickle.load(f))
                logging.debug("Loaded state from ~/.wd.state")
        except Exception as e:
            self.tasks = {}
            logging.debug("Could not load state from ~/.wd.state")
            logging.exception(e)

        self.beat_time = 0
        self.next_expiration = None

        try:
            self.sock = socket.socket(socket.AF_INET)
            self.sock.bind(('', port))
        except Exception as e:
            logging.critical("Failed to open socket.\n")
            logging.exception(e)
            raise

    def awake(self, signum, frame):
        logging.debug("Awake")
        try:
            if (time.time() - self.beat_time > 60.0):
                logging.debug("Beating")
                beat(server=self.wd_server, port=self.wd_port, signature='wd:primary')
                self.beat_time = time.time()
        except Exception as e:
            logging.warning("Failed to contact wd server")
            logging.exception(e)

        if self.next_expiration != None:
            logging.debug("Checking self.next_expiration")
            if self.next_expiration.expiration < time.time():
                logging.debug("Task: " + self.next_expiration.signature + " has expired")
                try:
                    logging.debug("Sending expiration notice")
                    comm.send_jarvis("wd." + socket.gethostname(), pwd.getpwuid( os.getuid() )[ 0 ], self.next_expiration.signature + " has expired.")
                except Exception as e:
                    logging.error("Failed to send expiration notice for " + str(self.next_expiration.signature))
                    logging.exception(e)
                self.dump_state()
        try:
            current_time = time.time()
            self.next_expiration = min([t for t in self.tasks.values() if (get_exp(t) > current_time)] , key=get_exp)
            signal.setitimer(signal.ITIMER_REAL, min(self.next_expiration.expiration - time.time(), self.beat_time + 60 - time.time()))
            logging.info("Next expiration is " + self.next_expiration.signature + " @ " + str(self.next_expiration.expiration))
        except Exception as e:
            self.next_expiration = None
            signal.setitimer(signal.ITIMER_REAL, max(self.beat_time + 60.0 - time.time(), 0.001))
            logging.info("No pending expiration")
            logging.exception(e)

    def dump_state(self):
        try:
            with open(os.path.expanduser("~/.wd.state"), 'wb', 0) as f:
                pickle.dump(self.tasks, f)
            logging.debug("Dumped state")
        except Exception as e:
            logging.warning("Failed to dump state")
            logging.exception(e)

    def run(self):
        self.sock.listen(1)
        logging.debug("Now listening on port " + str(self.port))
        self.awake()

        while True:
            try:
                for x in select.select([self.sock],[],[],1)[0]: #Readable sockets returned by select

                    if x == self.sock:
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
                                raise MessageError(message, "Bad Message")
                        if message.IsInitialized():
                            if message.HasField('beat'):
                                sig = message.beat.signature
                                try:
                                    t = self.tasks[sig]
                                    t.beat()
                                except KeyError:
                                    t = Task(sig)
                                    t.beat()
                                    self.tasks[sig] = t
                                logging.debug("Received beat: " + str(message.beat.signature) + "from: " + str(client_addr))
                                self.awake(signal.SIGALRM, None)
                                self.dump_state()
                            elif message.HasField('query'):
                                logging.debug("Received query: " + str(message.query.question))
                                response = watchdog_pb2.Message()
                                for s, t in self.tasks.iteritems():
                                    description = response.response.task.add()
                                    description.signature = s
                                    description.last = int(t.last)
                                    description.expected = int(t.expiration)
                                    description.mean = float(t.mean())
                                    description.deviation = float(t.deviation())
                                c.send(response.SerializeToString())
                                self.awake(signal.SIGALRM, None)
                            elif len(message.orders) > 0:
                                logging.debug("Received orders")
                                for cmd in message.orders:
                                    for fgt in cmd.to_forget:
                                        try:
                                            del self.tasks[fgt.signature]
                                        except Exception as e:
                                            logging.warning("Failed to delete: " + fgt.signature)
                                            logging.exception(e)
                                self.awake(signal.SIGALRM, None)
                                self.dump_state()
                            else:
                                logging.error("Unhandled Message: " + str(message))
                        else:
                            #unparseable message...
                            logging.error("Uninitialized Message: " + str(message))
                        c.close()
                    #self.awake(signal.SIGALRM, None)
            except KeyboardInterrupt: #ALSO CATCHES SIGINT
                pass
            except select.error as e:
                logging.debug("Select Interrupted")
            except MessageError as e:
                logging.warning("Message Error: " + e.error + " on " + e.message)
                logging.exception(e)


def daemon(port, dumpdir, wd_server, wd_port):

    logging.basicConfig(filename=os.path.expanduser("~/.wd.log"), level=logging.DEBUG, format='%(asctime)s: %(levelname)s: %(message)s')
    sys.excepthook = log_uncaught
    logging.info("Secondary watchdog server: " + wd_server + ":" + str(wd_port))

    wd = WatchDog(port, wd_server, wd_port)
    signal.signal(signal.SIGALRM, wd.awake)

    wd.run()
