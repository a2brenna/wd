#!/usr/bin/python3

import signal, sys, socket, select

RECV_BUFF_SIZE=4096

def daemon(port):
    inet = socket.socket(socket.AF_INET)
    try:
        inet.bind(('', port))
    except OSError as e:
        if (e.errno == 98): #Port in use
            print("Portin use!")
            print(e)
    inet.listen(1)

    while True:
        for x in select.select([inet],[],[],1)[0]: #Readable sockets returned by select
            c, client_addr = x.accept()
            message = c.recv(RECV_BUFF_SIZE)
            print(message)
