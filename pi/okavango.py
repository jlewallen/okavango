#!/usr/bin/python

from __future__ import print_function

import struct
import sys
import time
import syslog
import os
import glob
import datetime
import sqlite3

from utilities import log

class QueuedSamplesImporter:
    def get_queue_files(self, root):
        filename = "queue.log"
        if os.path.exists(os.path.join(root, filename)):
            stamp = datetime.datetime.fromtimestamp(time.time()).strftime('%Y%m%d%H%M%S')
            os.rename(os.path.join(root, filename), os.path.join(root, filename + "." + stamp ))
        return glob.glob(os.path.join(root, filename + ".*"))

    def process(self, root, database_path):
        db = sqlite3.connect(database_path, isolation_level="EXCLUSIVE")
        db.execute("CREATE TABLE IF NOT EXISTS samples (id INTEGER PRIMARY KEY AUTOINCREMENT, data TEXT)")

        for path in self.get_queue_files(root):
            log("Importing", path)
            with open(path) as f:
                rows = f.read().strip().split('\n')
                for row in rows:
                    db.execute("INSERT INTO samples (id, data) VALUES (NULL, ?)", [row])
            os.rename(path, os.path.join(root, 'processed', os.path.basename(path)))
        db.commit()
        db.close()

if __name__ == "__main__":
    path = "/home/pi/okavango/pi/lora-receiver"
    database_path = "/home/pi/okavango/pi/sensors.db"
    importer = QueuedSamplesImporter()
    importer.process(path, database_path)
