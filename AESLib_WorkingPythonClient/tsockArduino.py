
import sys

import base64

import socket

from Crypto.Cipher import AES
from Crypto import Random

#SECRET = "djsdifsafjoiasd838239jSDF*3jasjc"
SECRET = "1234567890123456"
BLOCK_SIZE = 16
PADDING = '~'

pad = lambda s: s + ( BLOCK_SIZE - len(s) % BLOCK_SIZE) * PADDING

class TSock:
    def __init__(self, IPPort = ('0.0.0.0', 12345) ):
        self.ipport = IPPort
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)


    def listen(self):
        self.bind = self.sock.bind( self.ipport )

        while True:
            data, addr = self.sock.recvfrom(1024);

            self.aes = AES.new( SECRET, AES.MODE_CBC, data[:16] )
            print "received message: ", self.aes.decrypt(data[16:])
            print "From: ", addr


    def send(self, message):

        iv = Random.new().read(BLOCK_SIZE)

        print base64.b64encode(iv)

        self.aes = AES.new( SECRET, AES.MODE_CBC, iv )

        self.sock.sendto( iv + self.aes.encrypt( pad(message)), self.ipport )




if __name__ == '__main__':
    print "0:", sys.argv[0]
    print "1:", sys.argv[1]

    if sys.argv[1] == 'server':
        s = TSock()
        s.listen()

    if sys.argv[1] == 'client':
        s = TSock()
        s.send( sys.argv[2] )
