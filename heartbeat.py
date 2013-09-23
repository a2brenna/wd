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
