#!/usr/bin/python

from xbee import ZigBee
import serial
import struct

def doPH(raw):
    return {
      'disolvedOxygen' : raw[0],
      'ph' : raw[1]
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
  0: doPH,
  1: altAir
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
    print dictionary
  except KeyboardInterrupt:
    break
        
serial.close()
