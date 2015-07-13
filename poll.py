
"""
Continuously read the serial port and process IO data received from a remote XBee.
"""

from xbee import ZigBee
import serial
import time
import glob

def on_radio(data):
	print data
	pass

devices = glob.glob("/dev/ttyUSB*") + glob.glob("/dev/tty.usbser*")
print "Devices", devices 
deviceName = devices[0]
device = serial.Serial(deviceName, 9600, timeout=5, writeTimeout=5)
xbee = ZigBee(device, callback=on_radio, escaped=False)

xbee.at(command="NT", parameter='\x10')
time.sleep(5)
xbee.at(command="ND")
time.sleep(5)
# xbee.at(command="MY")
# time.sleep(5)

# Continuously read and print packets
while True:
	try:
		time.sleep(5)
	except KeyboardInterrupt:
		break
        
device.close()

