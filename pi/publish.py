#!/usr/bin/python

from __future__ import print_function
from utilities import log
from rockblock import RockBlock
import json
import requests
import sqlite3

class Database:
    def __init__(self, database_path):
        self.db = sqlite3.connect(database_path, isolation_level="EXCLUSIVE")
        self.db.execute("CREATE TABLE IF NOT EXISTS published (sample_id INTEGER NOT NULL, destination TEXT NOT NULL)")
        self.db.execute("CREATE INDEX IF NOT EXISTS idx_published ON published (sample_id, destination)")

    def mark_as_done(self, ids, destination):
        new_rows = map(lambda id: [id, destination], ids)
        self.db.executemany("INSERT INTO published (sample_id, destination) VALUES (?, ?)", new_rows)

    def get_unpublished(self, destination):
        return self.db.execute("SELECT * FROM samples s WHERE id NOT IN (SELECT sample_id FROM published WHERE destination = ?) ORDER BY id DESC LIMIT 10", [destination])
    
    def close(self):
        self.db.commit()
        self.db.close()

class Publisher:
    def __init__(self, database):
        self.db = database

    def get_destination(self):
        return self.__class__.__name__

    def publish_sample(self, sample):
        return False

    def publish(self):
        destination = self.get_destination()

        published = []
        for sample in self.db.get_unpublished(destination):
            try:
                if self.publish_sample(sample[1]):
                    published.append(sample[0])
            except Exception as e:
                log(e)

        self.db.mark_as_done(published, destination)

class RockBlockPublisher(Publisher):
    def publish_sample(self, sample):
        rock_block = RockBlock(23)
        try:
            rock_block.check()
        finally:
            rock_block.close()

class HttpPublisher(Publisher):
    def to_json(self, sample):
        # [u'0', u'72370', u'3.377344', u'1020.000000', u'0.000000', u'234.639999', u'0.000000', u'0.000000', u'0.000000', u'1.000000', u'30.680000', u'102101.968750', u'47.246094']
        # log(columns)
        columns = sample.split(',')
        return {
            "t_local": columns[1],
            "data": {
                "battery": columns[2],
                "orp": columns[3],
                "ph": columns[4],
                "dissolved_oxygen": columns[5],
                # columns[6]
                "conductivity": columns[7],
                "salinity": columns[8],
                # columns[9]
                "air_temp": columns[10],
                "barometric_pressure": columns[11],
                "humidity": columns[12],
                "water temp": 0,
                "altitude": 0,
                "gps_lat": 0,
                "gps_long": 0,
                "speed": 0
            }
        }

    def publish_sample(self, sample):
        headers = {'Content-type': 'application/json', 'Accept': 'text/plain'}
        sample_json = self.to_json(sample)
        log(json.dumps(sample_json))
        url = 'http://httpbin.org/post'
        response = requests.post(url, data=json.dumps(sample_json), headers=headers, timeout=30)
        log(response.status_code)

if __name__ == "__main__":
    database_path = "/home/pi/okavango/pi/sensors.db"
    database = Database(database_path)

    publishers = [
        RockBlockPublisher(database),
        HttpPublisher(database)
    ]
    for publisher in publishers:
        log(publisher)
        publisher.publish()

    database.close()
