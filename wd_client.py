#!/usr/bin/python

import socket, os, time, socket, pwd, signal, sys, heartbeat

def t_wait(time_to_wait):
    end_time = time.time() + time_to_wait
    while time.time() < end_time:
        pid, status = os.waitpid(-1, os.WNOHANG)
        if pid != 0:
            return pid
        else:
            time.sleep(1)

def client(server, target_port, command, delay, heartrate, retry):
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
        signature = heartbeat.gen_sig(child_pid)
        while True:
            pid = t_wait(heartrate)
            if pid == child_pid:
                break
            else:
                attempts = 0
                while attempts < retry :
                    try:
                        heartbeat.beat(signature, server, target_port)
                        break
                    except:
                        attempts = attempts + 1
                        time.sleep(1)
                if attempts == retry:
                    break
