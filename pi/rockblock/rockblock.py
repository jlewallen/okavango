
"""
RockBLock Interface.
"""

import base64
import serial
import serial.threaded
import threading
import logging
import time
import glob
import Queue as queue

class ATException(Exception):
    pass

class ATProtocol(serial.threaded.LineReader):
    def __init__(self):
        super(ATProtocol, self).__init__();
        self.alive = True
        self.responses = queue.Queue()
        self.events = queue.Queue()
        self.event_responses = queue.Queue()
        self.lock = threading.Lock()
        self._event_thread = threading.Thread(target=self._run_event)
        self._event_thread.daemon = True
        self._event_thread.name = 'at-event'
        self._event_thread.start()
        self._awaiting_response_for = None

    def _run_event(self):
        while self.alive:
            try:
                self.handle_event(self.events.get())
            except:
                logging.exception('_run_event')

    def handle_line(self, line):
        if len(line) > 0:
            print '> ', line
            if line.startswith('+'):
                self.events.put(line)
            else:
                self.responses.put(line)

    def handle_event(self, event):
        pass

    def command_with_event_response(self, command):
        with self.lock:
            self._awaiting_response_for = command
            self.transport.write(b'{}\r'.format(command.encode(self.ENCODING, self.UNICODE_HANDLING)))
            response = self.event_responses.get()
            self._awaiting_response_for = None
            return response

    def command(self, command, response='OK', timeout=60):
        with self.lock:
            self.write_line(command)
            lines = []
            while True:
                try:
                    line = self.responses.get(timeout=timeout)
                    if line == response:
                        return lines
                    else:
                        lines.append(line)
                except queue.Empty:
                    raise ATException('AT command timeout ({!r})'.format(command))

class RockBlock(ATProtocol):
    def __init__(self):
        super(RockBlock, self).__init__()

    def connection_made(self, transport):
        super(RockBlock, self).connection_made(transport)

    def handle_event(self, event):
        if event.startswith('+SBDIX'):
            self.event_responses.put(event)
        elif event.startswith('+CSQ'):
            self.event_responses.put(event[5:])
        else:
            logging.warning('Unhandled Event: {!r}'.format(event))

    def setup(self):
        self.command("AT", response='OK')
        self.command("AT&K0", response='OK')

    def time(self):
        self.command("AT-MSSTM", response='OK')

    def location(self):
        self.command("AT-MSGEOS", response='OK')

    def get_serial_identifier(self):
        return self.command("AT+GSN")[1]

    def get_signal_strength(self):
        return self.command_with_event_response("AT+CSQ")

    def send_message(data):
        self.command("AT+SBDWT=Hello World")
        self.command("AT+SBDIX", response='OK')

if __name__ == '__main__':
    devices = glob.glob("/dev/ttyUSB*") + glob.glob("/dev/tty.usbser*")
    print "Devices", devices 
    deviceName = devices[0]
    device = serial.Serial(deviceName, 19200, timeout=5, writeTimeout=5)

    with serial.threaded.ReaderThread(device, RockBlock) as rock_block:
        rock_block.setup()
        print rock_block.get_serial_identifier()
        print rock_block.get_signal_strength()
            
    device.close()

