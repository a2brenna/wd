import watchdog_pb2, socket, os, psutil, logging, ssl

def gen_sig(pid=os.getpid()):
    p = psutil.Process(pid)
    return p.username + ":" + socket.gethostname() + ":" + p.name + ":" + str(pid)

def beat(server, port, signature=gen_sig()):
    logging.info("Sending beat with signature " + signature)
    message = watchdog_pb2.Message()
    message.beat.signature = signature

    try:
        s = ssl.wrap_socket(socket.socket(socket.AF_INET), server_side=False, cert_reqs=ssl.CERT_REQUIRED, certfile=os.path.expanduser("~/.ssl/key-cert.pem"), ca_certs=os.path.expanduser("~/.ssl/cacert.pem"))
        logging.debug("Attempting SSL Connection")
        s.connect((server,port))
        logging.debug("SSL Handshake complete")
        s.send(message.SerializeToString())
        s.shutdown(socket.SHUT_RDWR)
    except:
        logging.warning("Failed to send beat " + signature + " to " + server + ":" + str(port))

def query(server, port):
    logging.basicConfig(filename=os.path.expanduser("~/.wdclient.log"), level=logging.DEBUG, format='%(asctime)s: %(levelname)s: %(message)s')
    message = watchdog_pb2.Message()
    message.query.question = "Status"

    try:
        s = ssl.wrap_socket(socket.socket(socket.AF_INET), server_side=False, cert_reqs=ssl.CERT_REQUIRED, certfile=os.path.expanduser("~/.ssl/key-cert.pem"), ca_certs=os.path.expanduser("~/.ssl/cacert.pem"))
        logging.debug("Attempting SSL Connection")
        s.connect((server,port))
        logging.debug("SSL Handshake complete")
        s.send(message.SerializeToString())
        response = watchdog_pb2.Message()
        data = s.recv(4096)
        s.shutdown(socket.SHUT_RDWR)
    except:
        logging.error("Failed to query " + server + ":" + str(port))
        raise

    response.ParseFromString(data)
    return response.response

def export(server, port, export):
    logging.basicConfig(filename=os.path.expanduser("~/.wdclient.log"), level=logging.DEBUG, format='%(asctime)s: %(levelname)s: %(message)s')
    message = watchdog_pb2.Message()
    message.query.question = "Export"
    message.query.signature = export

    try:
        s = ssl.wrap_socket(socket.socket(socket.AF_INET), server_side=False, cert_reqs=ssl.CERT_REQUIRED, certfile=os.path.expanduser("~/.ssl/key-cert.pem"), ca_certs=os.path.expanduser("~/.ssl/cacert.pem"))
        logging.debug("Attempting SSL Connection")
        s.connect((server,port))
        logging.debug("SSL Handshake complete")
        s.send(message.SerializeToString())
        response = watchdog_pb2.Message()
        data = ""
        while True:
            more = s.recv(4096)
            if not more:
                break
            else:
                data = data + more
        s.shutdown(socket.SHUT_RDWR)
    except:
        logging.error("Failed to export " + export + " from " + server + ":" + str(port))
        raise

    response.ParseFromString(data)
    return response.response

def forget(server, port, signature):
    logging.basicConfig(filename=os.path.expanduser("~/.wdclient.log"), level=logging.DEBUG, format='%(asctime)s: %(levelname)s: %(message)s')
    message = watchdog_pb2.Message()
    command = message.orders.add()
    to_forget = command.to_forget.add()
    to_forget.signature = signature

    try:
        s = ssl.wrap_socket(socket.socket(socket.AF_INET), server_side=False, cert_reqs=ssl.CERT_REQUIRED, certfile=os.path.expanduser("~/.ssl/key-cert.pem"), ca_certs=os.path.expanduser("~/.ssl/cacert.pem"))
        logging.debug("Attempting SSL Connection")
        s.connect((server, port))
        logging.debug("SSL Handshake complete")
        s.send(message.SerializeToString())
        s.shutdown(socket.SHUT_RDWR)
    except:
        logging.warning("Failed to send forget: " + signature + " to " + server + ":" + str(port))
