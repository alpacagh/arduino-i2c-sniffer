"""
Convert from twi_sniff uart output into nice html view
Exports
    - WS enum class (wire status)
    - Decoder class
    - Encoder prototype
    - EncoderHtml implementation
Command-line usage:
    python2 convertToHtml.py [inputFile = stdin] >outputFile

source: https://github.com/alpacagh/arduino-i2c-sniffer
MIT license Copyright (c) 2016 alpacagm@gmail.com
https://opensource.org/licenses/MIT
"""


class WS:
    """
    WireStatus enum
    """
    invalid = 0
    high = 3
    low = 2
    rise = 1
    fall = -1


class Decoder:
    """
    Class to decode serial data from boards.
    """

    def __init__(self):
        self.SCL_BIT = 1
        self.SDA_BIT = 2
        self.SHIFT = ord('0')

    def setup_bits(self, scl_bit=None, sda_bit=None, shift=None):
        """
        Configure bit masks
        :param int scl_bit: one-bit mask for scl value
        :param int sda_bit: one-bit mask for sda value
        :param int shift:
        :return:
        """
        if scl_bit is not None:
            self.SCL_BIT = scl_bit
        if sda_bit is not None:
            self.SDA_BIT = sda_bit
        if shift is not None:
            self.SHIFT = shift

    def decode_status_char(self, status):
        """
        Decode single status
        :param char status: status character
        :return: {scl, sda}
        """
        status = ord(status) - self.SHIFT  # get numeric value
        return {"scl": status & self.SCL_BIT and WS.high or WS.low, "sda": status & self.SDA_BIT and WS.high or WS.low}

    def decode_status_line(self, line):
        """
        Decode transmission session into list of statuses and transitions
        :param string line:
        :return: statuses
        """
        prev_status = None
        statuses = []
        for stChar in line:
            status = self.decode_status_char(stChar)
            if prev_status:
                for wire in ("scl", "sda"):
                    if status[wire] == prev_status[wire]:
                        continue
                    elif status[wire] == WS.high:
                        if prev_status[wire] == WS.low or prev_status[wire] == WS.fall:
                            status[wire] = WS.rise
                    elif status[wire] == WS.low:
                        if prev_status[wire] == WS.high or prev_status[wire] == WS.rise:
                            status[wire] = WS.fall
            statuses.append(status)
            prev_status = status
        return statuses

    def decode_config_line(self, line):
        """
        :param string line:
        :return:
        """
        params = [val for val in line.rstrip("\n \t").split(':')]
        (scl, sda) = (int(val) for val in params[0:2])
        if len(params) > 2:
            shift = ord(params[2])
        else:
            shift = None
        self.setup_bits(scl, sda, shift)

    @staticmethod
    def interpret_status_chain(statuses):
        for status in statuses:
            if status["sda"] == WS.fall and status["scl"] == WS.high:
                status["int"] = "S"
            elif status["sda"] == WS.rise and status["scl"] == WS.high:
                status["int"] = "E"
            elif status["sda"] == WS.high and status["scl"] == WS.rise:
                status["int"] = "1"
            elif status["sda"] == WS.low and status["scl"] == WS.rise:
                status["int"] = "0"

    def process_line(self, line):
        """
        Process input line
        :param string line:
        :return:
        """
        if len(line) < 2:
            return None
        if line[0:2] == '!!':
            return line[2:].strip(' \n\t')
        elif line[0:2] == '>>':
            self.decode_config_line(line[2:])
            return None
        statuses = self.decode_status_line(line)
        self.interpret_status_chain(statuses)
        return statuses

    def parse_input(self, istream):
        """
        Process input file giving generator output
        :param file istream:
        :return:
        """
        for line in istream:
            yield self.process_line(line)


class Encoder:
    def encode_session(self, statuses): pass


class EncoderHtml(Encoder):
    def encode_line_event(self, status):
        return "<td class=\"kind_%s\"></td>" % (('falling', 'error', 'rising', 'low', 'high')[status + 1])

    def encode_int(self, status):
        try:
            return "<td class=\"interp\">%s</td>" % (status["int"])
        except KeyError:
            return "<td class=\"interp interp_none\"></td>"

    def encode_session(self, statuses):
        encoded_sda = [self.encode_line_event(statuses[i]["sda"]) for i in range(len(statuses))]
        encoded_scl = [self.encode_line_event(statuses[i]["scl"]) for i in range(len(statuses))]
        encoded_int = [self.encode_int(statuses[i]) for i in range(len(statuses))]
        return "<table><tr>\n%s\n</tr><tr>\n%s\n</tr><tr>\n%s\n</tr></table>" % (
            '\n'.join(encoded_sda), '\n'.join(encoded_scl), '\n'.join(encoded_int))

    def encode_label(self, label):
        return "<h2>%s</h2>\n" % label

    def encode_header(self):
        return '<html><head><link rel="stylesheet" href="style.css"></head><body>'

    def encode_footer(self):
        return '</body></html>'


if __name__ == "__main__":
    import sys
    import types

    if len(sys.argv) < 2:
        f = sys.stdin
    else:
        f = open(sys.argv[1], 'r')

    decoder = Decoder()
    encoder = EncoderHtml()
    print encoder.encode_header()
    for data in decoder.parse_input(f):
        if isinstance(data, types.ListType):
            print encoder.encode_session(data)
        elif isinstance(data, types.StringType):
            print encoder.encode_label(data)

    print encoder.encode_footer()
