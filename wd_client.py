#!/usr/bin/python

import socket, os, time, socket, pwd, signal, sys, heartbeat, logging

BACKOFF=0.01

def t_wait(time_to_wait):
    end_time = time.time() + time_to_wait
    while time.time() < end_time:
        pid, status = os.waitpid(-1, os.WNOHANG)
        if pid != 0:
            return pid
        else:
            time.sleep(1)

def client(server, target_port, command, delay, heartrate, retry):
    logging.basicConfig(filename=os.path.expanduser("~/.wdclient.log"), level=logging.DEBUG, format='%(asctime)s: %(levelname)s: %(message)s')
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
                        heartbeat.beat(server, target_port, signature)
                        break
                    except:
                        logging.error("Failed to beat")
                        time.sleep(BACKOFF * 2**attempts)
                        attempts = attempts + 1
                if attempts == retry:
                    break


def client2(server, target_port, pid, delay, heartrate, retry, signature):
    logging.basicConfig(filename=os.path.expanduser("~/.wdclient.log"), level=logging.DEBUG, format='%(asctime)s: %(levelname)s: %(message)s')
    import psutil
    start_time = psutil.Process(pid).create_time

    time.sleep(delay)
    if signature == None:
        signature = heartbeat.gen_sig(pid)
    while True:
        try:
            if start_time != psutil.Process(pid).create_time:
                break;
        except:
            break
        attempts = 0
        while attempts < retry :
            try:
                heartbeat.beat(server, target_port, signature)
                break
            except:
                logging.error("Failed to beat")
                time.sleep(BACKOFF * 2**attempts)
                attempts = attempts + 1
        if attempts == retry:
            break
        time.sleep(heartrate)
