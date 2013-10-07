import watchdog_pb2, socket, os, psutil, logging, ssl

def gen_sig(pid=os.getpid()):
    p = psutil.Process(pid)
    return p.username + ":" + socket.gethostname() + ":" + p.name + ":" + str(pid)

def beat(server, port, signature=gen_sig()):
    logging.info("Sending beat with signature " + signature)
    message = watchdog_pb2.Message()
    message.beat.signature = signature

    try:
        s = socket.socket(socket.AF_INET)
        #TODO SSL
        s = ssl.wrap_socket(s, server_side=False, cert_reqs=ssl.CERT_REQUIRED, certfile=os.path.expanduser("~/.ssl/key-cert.pem"), ca_certs=os.path.expanduser("~/.ssl/cacert.pem"))
        s.connect((server,port))
        s.send(message.SerializeToString())
        s.close
    except:
        logging.warning("Failed to send beat " + signature + " to " + server + ":" + str(port))

def query(server, port):
    message = watchdog_pb2.Message()
    message.query.question = "Status"

    try:
        s = socket.socket(socket.AF_INET)
        #TODO SSL
        s = ssl.wrap_socket(s, server_side=False, cert_reqs=ssl.CERT_REQUIRED, certfile=os.path.expanduser("~/.ssl/key-cert.pem"), ca_certs=os.path.expanduser("~/.ssl/cacert.pem"))
        s.connect((server,port))
        s.send(message.SerializeToString())
        response = watchdog_pb2.Message()
        data = s.recv(4096)
        s.close
    except:
        logging.error("Failed to query " + server + ":" + str(port))

    response.ParseFromString(data)
    return response.response

def forget(server, port, signature):
    message = watchdog_pb2.Message()
    command = message.orders.add()
    to_forget = command.to_forget.add()
    to_forget.signature = signature

    try:
        s = socket.socket(socket.AF_INET)
        #TODO SSL
        s = ssl.wrap_socket(s, server_side=False, cert_reqs=ssl.CERT_REQUIRED, certfile=os.path.expanduser("~/.ssl/key-cert.pem"), ca_certs=os.path.expanduser("~/.ssl/cacert.pem"))
        s.connect((server, port))
        s.send(message.SerializeToString())
        s.close
    except:
        logging.warning("Failed to send forget: " + signature + " to " + server + ":" + str(port))
