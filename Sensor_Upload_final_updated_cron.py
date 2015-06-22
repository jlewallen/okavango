#!/usr/bin/python

import datetime
import time
import json
import serial
import requests
import io
import os
import logging
import re
import sys
import ConfigParser

def fail(message):
  sys.stderr.write(message + "\n")
  sys.exit(2)

def validate_configuration(cfg):
  required = [ 'url', 'log_file', 'queue_file' ]
  for key in required:
    if not cfg.has_option('databoat', key) and len(cfg.get('databoat', key)):
      fail("Invalid configuration, missing databoat." + key)
  return cfg

def read_configuration():
  if not os.path.isfile(sys.argv[1]):
    fail("No " + sys.argv[1] + " file found!")
  cfg = ConfigParser.RawConfigParser()
  cfg.read(sys.argv[1])
  return validate_configuration(cfg)

logging.basicConfig(level=logging.DEBUG)
config = read_configuration()

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

# Save the sample locally, expecting the raw sample because we'll just append to a local CSV...
# may wanna make sure this doesn't fill up the SD card.
def save(timestamp, sample, parsed):
  queue_path = config.get('databoat', 'queue_file')
  log_path = config.get('databoat', 'log_file')
  new_file = not os.path.isfile(log_path)
  with open(log_path, "a") as f:
    if new_file:
      f.write("t_local,conductivity,salinity,ph,dissolved_oxygen,orp,water temp,humidity,air_temp,altitude,barometric_pressure,gps_lat,gps_long,speed,\n")
    f.write(timestamp + "," + sample + "\n")
  queue = {}
  if os.path.isfile(queue_path):
    try:
      with open(queue_path) as f:
        queue = json.load(f)
    except Exception as i:
      os.rename(queue_path, queue_path + "." + str(int(time.time())))
      print "Save Error", i
  queue[timestamp] = parsed
  with open(queue_path, "w") as f:
    json.dump(queue, f)
  return queue

# Upload a group of samples, converts a dict of samples keyed by timestamp to JSON and POSTs them to a URL.
# If successful returns an empty list, otherwise returns the samples to be uploaded on the following attempt.
def upload(samples):
  try:
    queue_path = config.get('databoat', 'queue_file')
    headers = {'Content-type': 'application/json', 'Accept': 'text/plain'}
    for time, sample in samples.iteritems():
      print json.dumps(sample)
      requests.post(config.get('databoat', 'url'), data=json.dumps(sample), headers=headers)
    os.remove(queue_path)
  except Exception as i:
    print "HTTP Error", i

# Main loop, read a sample, save locally, upload any buffered samples and sleep until the next time.
while True:
  latest_raw = read_sample()
  timestamp = str(int(time.time()))
  samples = save(timestamp, latest_raw, parse(timestamp, latest_raw))
  upload(samples)
  print "DONE"
  # Running in cron mode.
  if not config.has_option("databoat", "daemon") or config.get("databoat", "daemon") != "yes":
    sys.exit(0)
