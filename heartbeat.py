import watchdog_pb2, socket, os, psutil

def gen_sig(pid=os.getpid()):
    p = psutil.Process(pid)
    return p.username + ":" + socket.gethostname() + ":" + p.name + ":" + str(pid)

def beat(server, port, signature=gen_sig()):
    hb = watchdog_pb2.Heartbeat()
    hb.signature = signature

    s = socket.socket(socket.AF_INET)
    s.connect((server,port))
    s.send(hb.SerializeToString())
    s.close
