#!/usr/bin/python

import signal, sys, socket, select, watchdog_pb2, time, numpy, os, pwd, pickle, logging, traceback, ssl, utils
from hatcomm import jarvis_pb2, comm
from heartbeat import beat

RECV_BUFF_SIZE=4096
MIN_INTERVALS = 100
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
        if self.last != None:
            self.ivals.append(current - self.last)
        self.expiration = self.get_expiration()
        self.last = current
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

    def get_expiration(self):
        if len(self.ivals) > MIN_INTERVALS:
            mean = self.mean()
            std = self.deviation()
            return (time.time() + mean + (CONFIDENCE * std))
        else:
            return 2147483646

    def get_last(self):
        if self.last == None:
            return 0.0
        else:
            return self.last

def get_exp(t):
    return t.expiration

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

        for t in self.tasks.values():
            t.last = None
            if t.expiration > time.time():
                t.expiration = min(t.expiration, t.get_expiration())

        self.beat_time = 0
        self.next_expiration = None

        try:
            self.sock = ssl.wrap_socket(socket.socket(socket.AF_INET), server_side=True, certfile=os.path.expanduser("~/.ssl/key-cert.pem"), ca_certs=os.path.expanduser("~/.ssl/cacert.pem"), cert_reqs=ssl.CERT_REQUIRED)
            self.sock.bind(('', port))
        except Exception as e:
            logging.critical("Failed to open socket.\n")
            logging.exception(e)
            raise

    def awake(self, signum, frame):
        logging.debug("Awake")
        try:
            if (time.time() - self.beat_time > 60.0):
                if(socket.getaddrinfo(socket.gethostname(), self.port) != socket.getaddrinfo(self.wd_server, self.wd_port)):
                    logging.debug("Beating")
                    beat(server=self.wd_server, port=self.wd_port, signature='wd:primary')
                else:
                    logging.debug("Not beating to self")
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
                    comm.send_jarvis("wd:" + socket.gethostname(), "a2brenna", self.next_expiration.signature + " has expired.")
                except Exception as e:
                    logging.error("Failed to send expiration notice for " + str(self.next_expiration.signature))
                    logging.exception(e)
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
        self.dump_state()

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
        self.awake(signal.SIGALRM, None)

        while True:
            try:
                for x in select.select([self.sock],[],[],1)[0]: #Readable sockets returned by select

                    if x == self.sock:
                        logging.debug("Incoming connection")
                        try:
                            logging.debug("Initiating SSL Handshake")
                            c, client_addr = x.accept()
                            logging.debug("SSL Handshake Complete")
                            data = c.recv(RECV_BUFF_SIZE)
                            message = watchdog_pb2.Message()
                            try:
                                message.ParseFromString(data)
                            except:
                                logging.warning("Failed to parse incoming message.")
                            if message.IsInitialized():
                                if message.HasField('beat'):
                                    sig = message.beat.signature
                                    try:
                                        self.tasks[sig].beat()
                                    except KeyError:
                                        self.tasks[sig] = Task(sig)
                                    logging.debug("Received beat: " + str(message.beat.signature) + "from: " + str(client_addr))
                                elif message.HasField('query'):
                                    logging.debug("Received query: " + str(message.query.question))
                                    if (message.query.question == "Export"):
                                        try:
                                            t_sig = message.query.signature
                                            response = watchdog_pb2.Message()
                                            response.response.export
                                            export = response.response.export
                                            for interval in self.tasks[sig].ivals:
                                                logging.debug("Appending interval: " + str(float(interval)))
                                                export.interval.append(float(interval))
                                            logging.debug("Serialization complete")
                                            c.send(response.SerializeToString())
                                        except Exception as e:
                                            logging.exception(e)
                                    elif (message.query.question == "Status"):
                                        current_time = time.time()
                                        response = watchdog_pb2.Message()
                                        for s, t in self.tasks.iteritems():
                                            description = response.response.task.add()
                                            description.signature = s
                                            description.last = int(t.get_last())
                                            description.expected = int(t.expiration)
                                            description.time_to_expiration = int(t.expiration - current_time)
                                            description.mean = float(t.mean())
                                            description.deviation = float(t.deviation())
                                            description.beats = int(len(t.ivals) + 1)
                                        try:
                                            c.send(response.SerializeToString())
                                        except Exception as e:
                                            logging.exception(e)
                                elif len(message.orders) > 0:
                                    logging.debug("Received orders")
                                    for cmd in message.orders:
                                        for fgt in cmd.to_forget:
                                            try:
                                                del self.tasks[fgt.signature]
                                            except Exception as e:
                                                logging.warning("Failed to delete: " + fgt.signature)
                                                logging.exception(e)
                                else:
                                    logging.error("Unhandled Message: " + str(message))
                                self.awake(signal.SIGALRM, None)
                            else:
                                #unparseable message...
                                logging.error("Uninitialized Message: " + str(message))
                            c.shutdown(socket.SHUT_RDWR)
                            c.close()
                        except Exception as e:
                            logging.error("Failed to establish connection")
                            logging.exception(e)
                            continue
            except KeyboardInterrupt: #ALSO CATCHES SIGINT
                pass
            except select.error as e:
                logging.debug("Select Interrupted")
            except MessageError as e:
                logging.warning("Message Error: " + e.error + " on " + e.message)
                logging.exception(e)

    def shutdown(self, signum, frame):
        logging.debug("Shutting Down")
        try:
            self.dump_state()
            self.sock.shutdown(socket.SHUT_RDWR)
            self.sock.close()
        except Exception as e:
            logging.warning("Graceful shutdown failed")
            logging.exception(e)
            sys.exit(1)
        sys.exit(0)


def daemon(port, wd_server, wd_port):

    logging.basicConfig(filename=os.path.expanduser("~/.wd.log"), level=logging.DEBUG, format='%(asctime)s: %(levelname)s: %(message)s')
    sys.excepthook = utils.log_uncaught
    logging.info("Secondary watchdog server: " + wd_server + ":" + str(wd_port))

    wd = WatchDog(port, wd_server, wd_port)
    signal.signal(signal.SIGALRM, wd.awake)
    signal.signal(signal.SIGTERM, wd.shutdown)

    wd.run()
