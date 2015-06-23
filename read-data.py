"""
Continuously read the serial port and process IO data received from a remote XBee.
"""

from xbee import ZigBee
import serial
import struct

ser = serial.Serial('/dev/ttyUSB0', 9600)

xbee = ZigBee(ser)

# Continuously read and print packets
while True:
    try:
        response = xbee.wait_read_frame()
	data =  response['rf_data']
        print struct.unpack('ffffc', data)
    except KeyboardInterrupt:
        break
        
ser.close()
