import watchdog_pb2, socket, os, psutil

def gen_sig(pid=os.getpid()):
    p = psutil.Process(pid)
    return p.username + ":" + socket.gethostname() + ":" + p.name + ":" + str(pid)

def beat(server, port, signature=gen_sig()):
    message = watchdog_pb2.Message()
    message.beat.signature = signature

    s = socket.socket(socket.AF_INET)
    s.connect((server,port))
    s.send(message.SerializeToString())
    s.close

def query(server, port):
    message = watchdog_pb2.Message()
    message.query.question = "Status"

    s = socket.socket(socket.AF_INET)
    s.connect((server,port))
    s.send(message.SerializeToString())

    response = watchdog_pb2.Message()
    data = s.recv(4096)
    s.close

    response.ParseFromString(data)
    return response.response
