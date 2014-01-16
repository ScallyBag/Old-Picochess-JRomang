import serial
from struct import unpack

_DGTNIX_SEND_BRD = 0x42
_DGTNIX_MESSAGE_BIT = 0x80
_DGTNIX_BOARD_DUMP =  0x06

_DGTNIX_MSG_BOARD_DUMP = _DGTNIX_MESSAGE_BIT|_DGTNIX_BOARD_DUMP

_DGTNIX_SEND_UPDATE_NICE = 0x4b
_DGTNIX_FIELD_UPDATE =   0x0e
_DGTNIX_EMPTY = 0x00
_DGTNIX_WPAWN = 0x01
_DGTNIX_WROOK = 0x02
_DGTNIX_WKNIGHT = 0x03
_DGTNIX_WBISHOP = 0x04
_DGTNIX_WKING = 0x05
_DGTNIX_WQUEEN = 0x06
_DGTNIX_BPAWN =      0x07
_DGTNIX_BROOK  =     0x08
_DGTNIX_BKNIGHT =    0x09
_DGTNIX_BBISHOP =    0x0a
_DGTNIX_BKING   =    0x0b
_DGTNIX_BQUEEN  =    0x0c

_DGTNIX_SEND_CLK = 0x41
_DGTNIX_SEND_BRD = 0x42
_DGTNIX_SEND_UPDATE = 0x43
_DGTNIX_SEND_UPDATE_BRD = 0x44
_DGTNIX_SEND_SERIALNR = 0x45
_DGTNIX_SEND_BUSADDRESS = 0x46
_DGTNIX_SEND_TRADEMARK = 0x47
_DGTNIX_SEND_VERSION = 0x4d
_DGTNIX_SEND_UPDATE_NICE = 0x4b
_DGTNIX_SEND_EE_MOVES = 0x49
_DGTNIX_SEND_RESET = 0x40

_DGTNIX_SIZE_BOARD_DUMP = 67
_DGTNIX_NONE = 0x00
_DGTNIX_BOARD_DUMP = 0x06
_DGTNIX_BWTIME = 0x0d
_DGTNIX_FIELD_UPDATE = 0x0e
_DGTNIX_EE_MOVES = 0x0f
_DGTNIX_BUSADDRESS = 0x10
_DGTNIX_SERIALNR = 0x11
_DGTNIX_TRADEMARK = 0x12
_DGTNIX_VERSION = 0x13

piece_map = {
    _DGTNIX_EMPTY : '-',
    _DGTNIX_WPAWN : 'P',
    _DGTNIX_WROOK : 'R',
    _DGTNIX_WKNIGHT : 'N',
    _DGTNIX_WBISHOP : 'B',
    _DGTNIX_WKING : 'K',
    _DGTNIX_WQUEEN : 'Q',
    _DGTNIX_BPAWN : 'p',
    _DGTNIX_BROOK : 'r',
    _DGTNIX_BKNIGHT : 'n',
    _DGTNIX_BBISHOP : 'b',
    _DGTNIX_BKING : 'k',
    _DGTNIX_BQUEEN : 'q'
}

dgt_send_message_list = [_DGTNIX_SEND_CLK, _DGTNIX_SEND_BRD, _DGTNIX_SEND_UPDATE,
                         _DGTNIX_SEND_UPDATE_BRD, _DGTNIX_SEND_SERIALNR, _DGTNIX_SEND_BUSADDRESS, _DGTNIX_SEND_TRADEMARK,
                         _DGTNIX_SEND_VERSION, _DGTNIX_SEND_UPDATE_NICE, _DGTNIX_SEND_EE_MOVES, _DGTNIX_SEND_RESET]

class DGTBoard:
    def __init__(self, device):
        self.ser = serial.Serial(device,stopbits=serial.STOPBITS_ONE)

    def convertInternalPieceToExternal(self, c):
        if piece_map.has_key(c):
            return piece_map[c]

    def sendMessageToBoard(self, i):
        if i in dgt_send_message_list:
            self.ser.write(i)
        else:
            raise "Critical, cannot send - Unknown command: {0}".format(unichr(i))

    def dump_board(self, board):
        print len(board)
        pattern = '>'+'B'*len(board)
        buf = unpack(pattern, board)
        for square in xrange(0,len(board)):
#            if not square%8==0 and not square == 0:
#                print "|\n"
            print self.convertInternalPieceToExternal(buf[square])

#        for square in xrange(0, 8):
#            if not square%8==0:
#                if not square == 0:
#                    print "|\n"
#            print square
#            print self.convertInternalPieceToExternal(board[square])
#        print "\n"


    def read_message_from_board(self):
        header_len = 3
        header = self.ser.read(header_len)
        if not header:
            # raise
            raise Exception("Invalid First char in message")
        print len(header)
        pattern = '>'+'B'*header_len
        buf = unpack(pattern, header)
        print buf
#        print buf[0] & 128
#        if not buf[0] & 128:
#            raise Exception("Invalid message -2- readMessageFromBoard")
        command_id = buf[0] & 127
        print [command_id]
#
#        if header[1] & 128:
#            raise Exception ("Invalid message -4- readMessageFromBoard")
#
#        if header[2] & 128:
#            raise Exception ("Invalid message -6- readMessageFromBoard")

        message_length = (buf[1] << 7) + buf[2]
        message_length-=3
        print "len "+str(message_length)
        print message_length/8
        tmp1 = 0
        message = self.ser.read(64)
        print "len_message: {0}".format(len(message))
        print [message]

        if command_id == _DGTNIX_NONE:
            print "Received _DGTNIX_NONE from the board"

        elif command_id == _DGTNIX_BOARD_DUMP:
            print "DGTNIX_DUMP"
#            print len(message)
            self.dump_board(message)


if __name__ == "__main__":
    board = DGTBoard('/dev/cu.usbserial-00001004')

    line = []
    board.ser.write(chr(_DGTNIX_SEND_BRD))
    board.read_message_from_board()
#    while True:
#        sq = ser.read()
#        if chr(_DGTNIX_WQUEEN) ==sq:
#            print "WQueen"
