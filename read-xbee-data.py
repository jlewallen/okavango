#!/usr/bin/python

import serial
import struct
import sys
import time

from xbee import ZigBee
from okavango import SampleUploader

def phDO(raw):
    return {
      'ph' : raw[0],
      'do' : raw[1]
    }

def altAir(raw):
    return {
      'altitude' : raw[0],
      'air' : raw[1]
    }

def airWat(raw):
    return {
      'disolvedOxygen' : raw[0],
      'ph' : raw[1]
    }

def condOrp(raw):
    return {
      'disolvedOxygen' : raw[0],
      'ph' : raw[1]
    }

deserializers = {
  0: phDO,
  1: altAir,
  2: airWat,
  3: condOrp
}

serial = serial.Serial('/dev/ttyUSB0', 9600)
xbee = ZigBee(serial)

while True:
  try:
    response = xbee.wait_read_frame()
    data =  response['rf_data']
    payload = struct.unpack('ffffc', data)
    kind = int(payload[3])
    dictionary = deserializers[kind](payload)
    print payload, dictionary
    uploader = SampleUploader(sys.argv[1], None)
    timestamp = str(int(time.time()))
    samples = uploader.save(timestamp, None, dictionary)
    uploader.upload(samples)
  except KeyboardInterrupt:
    break
        
serial.close()
