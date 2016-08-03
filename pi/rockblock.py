
"""
RockBLock Interface.
"""

import base64
import serial
import serial.threaded
import threading
import time
import glob
import Queue as queue

from utilities import log

import RPi.GPIO as GPIO

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
            log('> ' + line)
            if line.startswith('+'):
                self.events.put(line)
            else:
                self.responses.put(line)

    def handle_event(self, event):
        pass

    def command_with_event_response(self, command, response='OK'):
        lines = self.command(command, response)
        log(lines)
        with self.lock:
            response = self.event_responses.get()
            return response

    def clear(self):
        while not self.responses.empty():
            self.responses.get()
        while not self.events.empty():
            self.events.get()
        while not self.event_responses.empty():
            self.event_responses.get()

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

    def send_message(self, data):
        self.clear()
        self.command("AT+SBDWT=Hello World", response='OK')
        self.command_with_event_response("AT+SBDIX")

if __name__ == '__main__':
    pin_sleep = 23

    GPIO.setwarnings(False)
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(pin_sleep, GPIO.OUT)

    log("Waking up RockBlock on pin %d..." % pin_sleep)

    GPIO.output(pin_sleep, True)

    log("Waiting 5 seconds...")

    time.sleep(5)

    log("Starting.")

    devices = glob.glob("/dev/ttyUSB*") + glob.glob("/dev/tty.usbser*")
    log("Devices", devices)
    deviceName = devices[0]
    device = serial.Serial(deviceName, 19200, timeout=5, writeTimeout=5)

    try:
        with serial.threaded.ReaderThread(device, RockBlock) as rock_block:
            rock_block.setup()
            log(rock_block.get_signal_strength())
            # rock_block.send_message(None)
    finally:
        log("Cleanup")
        # This returns GPIO to a state that turns the RockBLOCK back on, so we don't.
        # GPIO.cleanup()
        GPIO.output(pin_sleep, False)
        device.close()

