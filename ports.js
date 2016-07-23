var _ = require('lodash');
var SerialPort = require('serialport');

SerialPort.list(function(err, ports) {
    console.log(ports);
});

