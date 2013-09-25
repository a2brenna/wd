import watchdog_pb2, socket, os, psutil, logging

def gen_sig(pid=os.getpid()):
    p = psutil.Process(pid)
    return p.username + ":" + socket.gethostname() + ":" + p.name + ":" + str(pid)

def beat(server, port, signature=gen_sig()):
    logging.info("Sending beat with signature " + signature)
    message = watchdog_pb2.Message()
    message.beat.signature = signature

    try:
        s = socket.socket(socket.AF_INET)
        s.connect((server,port))
        s.send(message.SerializeToString())
        s.close
    except:
        logging.warning("Failed to send beat " + signature + " to " + server + ":" + port)

def query(server, port):
    message = watchdog_pb2.Message()
    message.query.question = "Status"

    try:
        s = socket.socket(socket.AF_INET)
        s.connect((server,port))
        s.send(message.SerializeToString())
        response = watchdog_pb2.Message()
        data = s.recv(4096)
        s.close
    except:
        loggging.error("Failed to query " + server + ":" + str(port))

    response.ParseFromString(data)
    return response.response
