import watchdog_pb2, socket, os, psutil

def gen_sig(pid=os.getpid()):
    p = psutil.Process(pid)
    return p.username + ":" + socket.gethostname() + ":" + p.name + ":" + str(pid)

def beat(signature=gen_sig(), server, port):
    hb = watchdog_pb2.Heartbeat()
    hb.signature = signature

    s = socket.socket(socket.AF_INET)
    s.connect((server,port))
    s.send(hb.SerializeToString())
    s.close
