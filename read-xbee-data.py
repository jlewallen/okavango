#!/usr/bin/python

import serial
import struct
import sys
import time
import logging

from xbee import ZigBee
from okavango import SampleUploader

logging.basicConfig(level=logging.DEBUG)

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

serial = serial.Serial('/dev/ttyUSB0', 9600)
xbee = ZigBee(serial)

while True:
  try:
    response = xbee.wait_read_frame()
    data =  response['rf_data']
    payload = struct.unpack('ffffb', data)
    kind = payload[4]
    dictionary = deserializers[kind](payload)
    print payload, dictionary
    timestamp = str(int(time.time()))
    sample = {
      "t_local": timestamp,
      "data": dictionary
    }
    uploader = SampleUploader(sys.argv[1], None)
    samples = uploader.save(timestamp, None, sample)
    uploader.upload(samples)
  except KeyboardInterrupt:
    break
        
serial.close()
