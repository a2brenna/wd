#!/usr/bin/python

import socket, watchdog_pb2, os, time, socket, pwd

def client(server, target_port, command, delay, heartrate):
    try:
        child_pid = os.fork()
    except OSError as e:
        raise
    if not child_pid:
        #IS CHILD
        os.execv("/bin/sh", ['sh', '-c'] + command)

    else: #IS PARENT
        time.sleep(delay)
        while True:
            inet = socket.socket(socket.AF_INET)
            inet.connect((server, target_port))
            beat = watchdog_pb2.Heartbeat()
            beat.signature = pwd.getpwuid( os.getuid() )[ 0 ] + ":" + socket.gethostname() + ":" + command[0] + ":" + str(child_pid)
            pid, status = os.waitpid(-1, os.WNOHANG)
            if pid != 0:
                break
            inet.send(beat.SerializeToString())
            inet.close()
            time.sleep(heartrate)
