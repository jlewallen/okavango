#!/usr/bin/python

from __future__ import print_function

import serial
import struct
import sys
import time
import logging
import daemon
import syslog

from xbee import ZigBee
from okavango import SampleUploader

logging.basicConfig(level=logging.DEBUG)
syslog.openlog(logoption=syslog.LOG_PID)

def phDO(raw):
    return {
      'ph' : raw[0],
      'do' : raw[1]
    }

def airAlt(raw):
    return {
      'humidity' : raw[0],
      'airTemp' : raw[1],
      'pressure' : raw[2]
    }

def airWat(raw):
    return {
      'waterTemp' : raw[0],
      'humidity' : raw[1],
      'airTemp' : raw[2]
    }

def condOrp(raw):
    return {
      'tds' : raw[0],
      'salinity' : raw[1],
      'orp' : raw[2]
    }

deserializers = {
  0: phDO,
  1: condOrp,
  2: airWat,
  3: airAlt
}

def log(*objects):
  print(*objects)
  for i in objects:
    syslog.syslog(str(i))

class XbeeListener(daemon.Daemon):
  def __init__(self, pidfile, stdin='/dev/null', stdout='/dev/null', stderr='/dev/null'):
    daemon.Daemon.__init__(self, pidfile, stdin, stdout, stderr)

  def run(self):
    while True:
      try:
        device = serial.Serial('/dev/ttyUSB0', 9600)
        xbee = ZigBee(device)

        while True:
          try:
            response = xbee.wait_read_frame()
            data =  response['rf_data']
            payload = struct.unpack('ffffb', data)
            kind = payload[4]
            dictionary = deserializers[kind](payload)
            log(payload, dictionary)
            timestamp = str(int(time.time()))
            sample = {
              "t_local": timestamp,
              "data": dictionary
            }
            uploader = SampleUploader(sys.argv[1], None)
            samples = uploader.save(timestamp, None, sample)
            uploader.upload(samples)
          except KeyboardInterrupt:
            sys.exit(0)
          except Exception as i:
            log(i)
            time.sleep(1)
                
        device.close()
      except Exception as i:
        log(i)
        time.sleep(1)

if __name__ == "__main__":
  log("Starting...")
  listener = XbeeListener("/var/run/xbee.pid")
  if "start" in sys.argv:
    listener.start()
  if "restart" in sys.argv:
    listener.restart()
  if "stop" in sys.argv:
    listener.stop()
  else:
    listener.run()
