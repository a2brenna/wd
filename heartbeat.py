import watchdog_pb2, socket

def beat(signature, server, port):
    hb = watchdog_pb2.Heartbeat()
    hb.signature = signature

    s = socket.socket(socket.AF_INET)
    s.connect((server,port))
    s.send(hb.SerializeToString())
    s.close
