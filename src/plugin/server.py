import os
import logging
import SocketServer
import struct

try:
    loglevel = int(os.getenv("E2KODI_DEBUG_LVL", logging.ERROR))
except Exception:
    loglevel = logging.ERROR
print "E2KODI_DEBUG_LVL = ", loglevel

logging.basicConfig(level=loglevel, format='%(name)s: %(message)s',)

class KodiExtRequestHandler(SocketServer.BaseRequestHandler):

    def __init__(self, request, client_address, server):
        self.logger = logging.getLogger('KodiExtRequestHandler')
        SocketServer.BaseRequestHandler.__init__(self, request, client_address, server)

    def handle(self):
        hlen = struct.calcsize('ibi')
        header = self.request.recv(hlen)
        opcode, status, datalen = struct.unpack('ibi',header)
        if datalen > 0:
            data = self.request.recv(datalen)
        else:
            data = None
        self.logger.debug('recv()-> opcode = %d, status = %d, data = %s', opcode, status, str(data))
        status, data = self.handle_request(opcode, status, data)
        if data is not None:
            datalen = len(data)
        else:
            datalen = 0
        self.logger.debug('send()-> opcode = %d, status = %d, data = %s', opcode, status, str(data))
        header = struct.pack('ibi', opcode, status, datalen)
        self.request.send(header)
        if datalen > 0:
            self.request.send(data)

    def handle_request(self, opcode, status, data):
        return True, None


class UDSServer(SocketServer.UnixStreamServer):

    def __init__(self, server_address, handler_class=KodiExtRequestHandler):
        self.logger = logging.getLogger('UDSServer')
        self.allow_reuse_address = True
        SocketServer.UnixStreamServer.__init__(self, server_address, handler_class)
