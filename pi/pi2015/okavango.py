#!/usr/bin/python

from __future__ import print_function

import requests
import json
import logging
import ConfigParser
import sys
import os
import syslog

def fail(message):
  sys.stderr.write(message + "\n")
  sys.exit(2)

def log(*objects):
  print(*objects)
  for i in objects:
    syslog.syslog(str(i))

class SampleUploader:
  def __init__(self, path, header):
    self.config = self.read_configuration(path)
    self.header = header

  def validate_configuration(self, cfg):
    required = [ 'url', 'log_file', 'queue_file' ]
    for key in required:
      if not cfg.has_option('databoat', key) and len(cfg.get('databoat', key)):
        fail("Invalid configuration, missing databoat." + key)
    return cfg

  def read_configuration(self, path):
    if not os.path.isfile(path):
      fail("No " + path + " file found!")
    cfg = ConfigParser.RawConfigParser()
    cfg.read(path)
    return self.validate_configuration(cfg)

  # Save the sample locally, expecting the raw sample because we'll just append to a local CSV...
  # may wanna make sure this doesn't fill up the SD card.
  def save(self, timestamp, sample, parsed):
    queue_path = self.config.get('databoat', 'queue_file')
    log_path = self.config.get('databoat', 'log_file')
    new_file = not os.path.isfile(log_path)
    if sample:
      with open(log_path, "a") as f:
        if new_file:
          f.write(self.header)
        f.write(timestamp + "," + sample + "\n")
    queue = {}
    if os.path.isfile(queue_path):
      try:
        with open(queue_path) as f:
          queue = json.load(f)
      except Exception as i:
        os.rename(queue_path, queue_path + "." + str(int(time.time())))
        log("Save Error", i)
    queue[timestamp] = parsed
    with open(queue_path, "w") as f:
      json.dump(queue, f)
    return queue

  # Upload a group of samples, converts a dict of samples keyed by timestamp to JSON and POSTs them to a URL.
  # If successful returns an empty list, otherwise returns the samples to be uploaded on the following attempt.
  def upload(self, samples):
    try:
      queue_path = self.config.get('databoat', 'queue_file')
      headers = {'Content-type': 'application/json', 'Accept': 'text/plain'}
      for time, sample in samples.iteritems():
        log(json.dumps(sample))
        requests.post(self.config.get('databoat', 'url'), data=json.dumps(sample), headers=headers, timeout=30)
      os.remove(queue_path)
    except Exception as i:
      log("HTTP Error", i)

