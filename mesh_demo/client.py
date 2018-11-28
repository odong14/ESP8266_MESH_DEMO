# chat_client.py
#!/usr/bin/python

import sys
import socket
import select
import binascii
Mac_Address_local  = 'c0a80115581b'

def chat_client():
    if(len(sys.argv) < 3) :
        print 'Usage : python chat_client.py hostname port'
        sys.exit()

    host = sys.argv[1]
    port = int(sys.argv[2])
     
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(2)
     
    # connect to remote host
    try :
        s.connect((host, port))
    except :
        print 'Unable to connect'
        sys.exit()
     
    print 'Connected to remote host. You can start sending messages'
    sys.stdout.write('[Me] '); sys.stdout.flush()
     
    while 1:
        socket_list = [sys.stdin, s]
         
        # Get the list sockets which are readable
        ready_to_read,ready_to_write,in_error = select.select(socket_list , [], [])
         
        for sock in ready_to_read:             
            if sock == s:
                # incoming message from remote server, s
                data = sock.recv(1024)
                if not data :
                    print '\nDisconnected from chat server'
                    sys.exit()
                else :
                    #print data
                    sys.stdout.write(data)
                    sys.stdout.write('[Me] '); sys.stdout.flush()     
            
            else :
                # user entered a message
                msg = sys.stdin.readline()
                data = str(msg).split('/')
                print data
                Panjang_Paket = len(data[1])+16
                _header = '4008'+str(hex(Panjang_Paket).split('x')[1])+'00'
                test = bytearray()
                test[0:4]= binascii.unhexlify(_header)
                test[4:10]= binascii.unhexlify(Mac_Address_local)
                test[10:16]= binascii.unhexlify(data[0])
                test[16:]= data[1]
                data_sent = test[0:4] + test[4:10]+ test[10:16]+ test[16:]
                print 'Payload: ' + binascii.hexlify(data_sent[0:]) 
                s.send(data_sent)
                sys.stdout.write('[Me] '); sys.stdout.flush() 

if __name__ == "__main__":

    sys.exit(chat_client())