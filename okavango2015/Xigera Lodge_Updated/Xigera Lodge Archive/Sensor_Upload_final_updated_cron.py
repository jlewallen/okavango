#!/usr/bin/python

import datetime
import time
import serial
import io
import os
import logging
import re
import sys

from okavango import SampleUploader

def fail(message):
  sys.stderr.write(message + "\n")
  sys.exit(2)

logging.basicConfig(level=logging.DEBUG)
arduino = serial.Serial('/dev/ttyACM0', 115200)

def is_sample_valid(raw):
  return len(filter(len, raw.split(","))) == 13

def sanitize_sample(raw):
  return re.sub('[^0-9.,-]', '', raw)
  
# Read one sample from the Arduino, return the sample as a string with CSVs
def read_sample():
  buf = []
  while True:
    for c in arduino.read():
      buf.append(c)
      if c == '\n':
        print "RAW", buf
        line = ''.join(',' if i in ['\r', '\n'] else i for i in buf)
        print "LINE", line
        if is_sample_valid(line):
          return sanitize_sample(line)
        buf = []
        break

# Parse the sample prior to upload.
def parse(timestamp, sample):
  columns = filter(len, sample.split(","))
  return {
    "t_local": timestamp,
    "data": {
      "conductivity": columns[0],
      "salinity": columns [1],
      "ph": columns[2],
      "dissolved_oxygen": columns[3],
      "orp": columns[4],
      "water temp": columns[5],
      "humidity": columns[6],
      "air_temp": columns [7],
      "altitude": columns[8],
      "barometric_pressure": columns[9],
      "gps_lat": columns[10],
      "gps_long": columns [11],
      "speed": columns[12]
    }
  }

# Main loop, read a sample, save locally, upload any buffered samples and sleep until the next time.
while True:
  uploader = SampleUploader(sys.argv[1], "t_local,conductivity,salinity,ph,dissolved_oxygen,orp,water temp,humidity,air_temp,altitude,barometric_pressure,gps_lat,gps_long,speed,\n")
  latest_raw = read_sample()
  timestamp = str(int(time.time()))
  samples = uploader.save(timestamp, latest_raw, parse(timestamp, latest_raw))
  uploader.upload(samples)
  print "DONE"
  # Running in cron mode.
  if not config.has_option("databoat", "daemon") or config.get("databoat", "daemon") != "yes":
    sys.exit(0)
