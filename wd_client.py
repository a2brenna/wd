#!/usr/bin/python

import socket, watchdog_pb2, os, time, socket, pwd, signal, sys

MAX_ATTEMPTS = 5

def t_wait(time_to_wait):
    end_time = time.time() + time_to_wait
    while time.time() < end_time:
        pid, status = os.waitpid(-1, os.WNOHANG)
        if pid != 0:
            return pid
        else:
            time.sleep(1)

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
            pid = t_wait(heartrate)
            if pid == child_pid:
                break
            else:
                attempts = 0
                while attempts < MAX_ATTEMPTS:
                    try:
                        inet = socket.socket(socket.AF_INET)
                        inet.connect((server, target_port))
                        inet.send(beat.SerializeToString())
                        inet.close()
                        break
                    except:
                        print("Could not reach server, retrying")
                        attempts = attempts + 1
                        time.sleep(1)
                if attempts == MAX_ATTEMPTS:
                    print("Could not reach server after: " + str(MAX_ATTEMPTS) + " attempts, exiting.")
                    break
