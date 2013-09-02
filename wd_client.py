#!/usr/bin/python

import socket, watchdog_pb2, os, time, socket, pwd, signal

def client(server, target_port, command, delay, heartrate):
    try:
        child_pid = os.fork()
    except OSError as e:
        raise
    if not child_pid:
        #IS CHILD
        os.execv("/bin/sh", ['sh', '-c'] + command)

    else: #IS PARENT
        #Mask off signals
        for sig in [signal.SIGINT]:
            signal.signal(sig, signal.SIG_IGN)
        time.sleep(delay)
        beat = watchdog_pb2.Heartbeat()
        beat.signature = pwd.getpwuid( os.getuid() )[ 0 ] + ":" + socket.gethostname() + ":" + command[0] + ":" + str(child_pid)
        while True:
            inet = socket.socket(socket.AF_INET)
            inet.connect((server, target_port))
            pid, status = os.waitpid(-1, os.WNOHANG)
            if pid != 0:
                break
            inet.send(beat.SerializeToString())
            inet.close()
            time.sleep(heartrate)
